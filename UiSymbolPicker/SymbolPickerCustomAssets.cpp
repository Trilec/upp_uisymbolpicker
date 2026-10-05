#include "SymbolPickerCustomAssets.h"
#include "SymbolPickerImageRender.h"
#include "SymbolPickerProjectIo.h"
#include "SymbolPickerImlExport.h"
#include "SymbolPickerCollectionProjection.h"
#include <plugin/png/png.h>

namespace Upp {

bool IsSymbolPickerAssetFile(const String& path)
{
	String ext = ToLower(GetFileExt(path));
	return ext == ".svg" || ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif";
}

static void ScanAssets(SymbolPickerCatalog& catalog, const String& folder, const String& relative)
{
	Vector<String> paths;
	for(FindFile f(AppendFileName(folder, "*")); f; ++f)
		if(f.GetName() != "." && f.GetName() != "..") paths.Add(f.GetPath());
	Sort(paths);
	for(const String& path : paths) {
		String name = AppendFileName(relative, GetFileName(path));
		if(DirectoryExists(path)) { ScanAssets(catalog, path, name); continue; }
		if(!IsSymbolPickerAssetFile(path)) continue;
		SymbolPickerIconEntry entry;
		entry.catalog_id = "assets/" + UnixPath(name);
		if(catalog.FindByCatalogId(entry.catalog_id)) continue;
		entry.source_id = entry.catalog_id;
		entry.category = ToLower(GetFileExt(path)) == ".svg" ? "SVG" : "Images";
		Vector<String> subfolders = Split(UnixPath(name), '/');
		if(subfolders.GetCount() > 2) entry.category << "/" << subfolders[1];
		entry.display_name = GetFileTitle(path);
		entry.source_symbol = entry.display_name;
		entry.external_file = GetFullPath(path);
		catalog.Add(entry);
	}
}

void LoadSymbolPickerAssetFolders(SymbolPickerCatalog& catalog, const String& folder)
{
	if(DirectoryExists(folder)) ScanAssets(catalog, folder, String());
}

bool RunSymbolPickerCustomAssetSmokeTests(String& error)
{
	auto Fail = [&](const String& message) { error = message; return false; };
	String dir = AppendFileName(GetTempPath(), "symbolpicker_custom_" + AsString(Random64()));
	RealizeDirectory(dir);
	String svg = AppendFileName(dir, "wide.svg"), png = AppendFileName(dir, "wide.png");
	String project_path = AppendFileName(dir, "custom.symbolpicker");
	SaveFile(svg, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 40 20\"><rect width=\"40\" height=\"20\" fill=\"#000\"/></svg>");
	ImageBuffer pixels(40, 20); Fill(~pixels, RGBA{0, 0, 0, 255}, pixels.GetLength());
	PNGEncoder().SaveFile(png, Image(pixels));
	SymbolPickerProject project;
	for(const String& path : {svg, png}) {
		auto& asset = project.custom_assets.Add();
		asset.catalog_id = "custom/" + GetFileName(path);
		asset.file_path = path; asset.display_name = GetFileTitle(path);
	}
	project.collections.Add().name = "Custom";
	for(const auto& asset : project.custom_assets) {
		auto& ref = project.collections[0].items.Add();
		ref.catalog_id = ref.source_id = asset.catalog_id;
		ref.alias = "ICON_" + GetFileTitle(asset.file_path);
		ref.size = 48;
	}
	if(!SaveSymbolPickerProjectJson(project, project_path, error)) return false;
	SymbolPickerProject loaded;
	if(!LoadSymbolPickerProjectJson(project_path, loaded, error)) return false;
	if(loaded.custom_assets.GetCount() != 2 || loaded.custom_assets[0].file_path != GetFullPath(svg)) return Fail("Custom references did not round-trip.");
	if(LoadFile(project_path).Find("<svg") >= 0 || LoadFile(project_path).Find("wide.svg") < 0) return Fail("Custom project embedded artwork or lost relative paths.");
	SymbolPickerCatalog catalog; catalog.SetCustomAssets(loaded.custom_assets);
	if(catalog.Filter("All", "", SymbolPickerIconStyle::Outlined, 0).GetCount() != 1 || catalog.Filter("All", "", SymbolPickerIconStyle::Sharp, 1).GetCount() != 1) return Fail("SVG and raster asset views were mixed or raster images were hidden by symbol style.");
	for(const auto& asset : loaded.custom_assets) {
		const auto* entry = catalog.FindByCatalogId(asset.catalog_id);
		Image image = entry ? RenderSymbolPickerIconImage(*entry, 48, Null) : Image();
		if(image.GetSize() != Size(48, 48) || image[0][24].a != 0 || image[24][24].a == 0) return Fail("Custom rendering lost dimensions, centering or aspect ratio.");
	}
	String iml, header; Vector<String> warnings;
	if(!BuildSymbolPickerUppImlLibrary(loaded, catalog, SymbolPickerExportScope::AllCollections, "Custom.iml", iml, header, &warnings) || !warnings.IsEmpty()) return Fail("Custom assets did not export to IML.");
	FileDelete(svg); catalog.SetCustomAssets(loaded.custom_assets);
	SymbolPickerCollectionProjection projection; projection.Rebuild(loaded.collections[0], catalog, String());
	if(catalog.FindByCatalogId("custom/wide.svg")->available || !projection.Model().Get(0).has_metadata) return Fail("Missing custom file was not retained and flagged.");
	FileDelete(png); FileDelete(project_path); DirectoryDelete(dir);
	return true;
}
}
