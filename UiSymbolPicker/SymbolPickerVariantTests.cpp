#include "SymbolPickerGeneratedCatalog.h"
#include "SymbolPickerProjectIo.h"
#include "SymbolPickerImageRender.h"
#include "SymbolPickerExport.h"
#include "SymbolPickerImlExport.h"
#include "SymbolPickerCommands.h"

namespace Upp {

bool RunSymbolPickerVariantTests(const SymbolPickerCatalog& catalog, String& error)
{
	auto Fail = [&](const String& message) { error = message; return false; };
	SymbolPickerProject project;
	project.project_name = "Optical and fill checks"; project.default_optical_size = 48; project.default_filled = true;
	project.output_base_name = "VariantIcons"; project.active_collection_index = 0;
	auto& collection = project.collections.Add(); collection.name = "Variant icons";
	String outline20, filled20, outline48;
	int variant_count = 0;
	for(auto style : {SymbolPickerIconStyle::Outlined, SymbolPickerIconStyle::Rounded, SymbolPickerIconStyle::Sharp})
		for(int optical : {20, 48}) for(bool filled : {false, true}) {
			Vector<int> matches = catalog.Filter("All", "", style, 0, true, optical, filled);
			if(matches.GetCount() < 5000) return Fail("An optical/fill combination did not expose the complete library.");
			int total = 0;
			for(const auto& category : catalog.GetCategories(style, 0, optical, filled)) total += category.icon_count;
			if(total != matches.GetCount()) return Fail("Optical/fill category counts differ from reachable items.");
			const SymbolPickerIconEntry* sample = nullptr;
			for(int i : matches) {
				const auto& entry = catalog.GetIcons()[i];
				if(!entry.image_factory && entry.external_file.IsEmpty() && (entry.optical_size != optical || entry.filled != filled))
					return Fail("The library mixed optical or fill designs.");
				if(entry.source_id.EndsWith("/home") && !entry.image_factory) sample = &entry;
			}
			if(!sample) return Fail("Could not find Home in an optical/fill combination.");
			String svg;
			if(!DecodeGeneratedSymbolPickerSvg(sample->catalog_id, svg) || !IsSVG(svg))
				return Fail("A selected optical/fill SVG could not be decoded.");
			if(style == SymbolPickerIconStyle::Outlined) {
				if(optical == 20 && !filled) outline20 = svg;
				if(optical == 20 && filled) filled20 = svg;
				if(optical == 48 && !filled) outline48 = svg;
			}
			for(int px : {48, 64}) if(RenderSymbolPickerIconImage(*sample, px, Null).GetSize() != Size(px, px))
				return Fail("Optical design was confused with output pixel size.");
			auto& item = collection.items.Add(); item.catalog_id = sample->catalog_id; item.source_id = sample->source_id;
			item.alias = "HOME_" + AsString((int)style) + "_" + AsString(optical) + (filled ? "_FILLED" : "_UNFILLED"); item.size = 48;
			++variant_count;
		}
	if(outline20 == filled20 || outline20 == outline48) return Fail("Fill/optical selection reused the same artwork.");
	const SymbolPickerIconEntry* legacy = nullptr;
	for(const auto& entry : catalog.GetIcons()) if(entry.source_id.EndsWith("/home") && entry.style == SymbolPickerIconStyle::Outlined && entry.optical_size == 24) { legacy = &entry; break; }
	if(!legacy) return Fail("Legacy 24 optical artwork was removed.");
	auto& old = collection.items.Add(); old.catalog_id = legacy->catalog_id; old.source_id = legacy->source_id; old.alias = "LEGACY_HOME"; old.size = 48;
	String folder = GetExeDirFile("variant-tests"); RealizeDirectory(folder);
	String path = AppendFileName(folder, "Variants.symbolpicker");
	if(!SaveSymbolPickerProjectJson(project, path, error)) return false;
	SymbolPickerProject loaded;
	if(!LoadSymbolPickerProjectJson(path, loaded, error)) return false;
	if(loaded.default_optical_size != 48 || !loaded.default_filled || loaded.collections[0].items.GetCount() != 13)
		return Fail("Project round-trip lost optical/fill defaults or items.");
	for(int i = 0; i < 13; ++i) if(loaded.collections[0].items[i].catalog_id != collection.items[i].catalog_id)
		return Fail("Project round-trip changed selected artwork identity.");
	SymbolPickerModel model; model.LoadProject(loaded); SymbolPickerCommandStack commands;
	if(!commands.Execute(MakeSymbolPickerSetMaterialVariantCommand(20, false), model) || !commands.Undo(model)
	   || model.GetOpticalSize() != 48 || !model.GetFilled()) return Fail("Undo lost optical/fill settings.");
	Vector<String> warnings; int written = 0, skipped = 0;
	if(!ExportSymbolPickerPngFiles(loaded, catalog, SymbolPickerExportScope::AllCollections, AppendFileName(folder, "png"), &warnings, &written, &skipped)
	   || written != 13 || skipped || !warnings.IsEmpty()) return Fail("Optical/fill PNG export failed.");
	if(!ExportSymbolPickerSvgFiles(loaded, catalog, SymbolPickerExportScope::AllCollections, AppendFileName(folder, "svg"), &warnings, &written, &skipped)
	   || written != 13 || skipped || !warnings.IsEmpty()) return Fail("Optical/fill SVG export failed.");
	String iml, header, iml_path = AppendFileName(folder, "VariantIcons.iml");
	if(!BuildSymbolPickerUppImlLibrary(loaded, catalog, SymbolPickerExportScope::AllCollections, iml_path, iml, header, &warnings)
	   || !warnings.IsEmpty() || !SaveFile(iml_path, iml) || !SaveFile(ForceExt(iml_path, ".h"), header))
		return Fail("Optical/fill IML/header export failed.");
	// A missing explicit variant must never fall back to the older source design.
	SymbolPickerProject missing = model.ExportProject(); missing.collections[0].items[0].catalog_id = "missing/home/outlined/20/filled";
	warnings.Clear(); auto exports = BuildSymbolPickerExportItems(missing, catalog, SymbolPickerExportScope::AllCollections, &warnings);
	if(warnings.IsEmpty()) return Fail("A missing explicit variant silently resolved to another design.");
	SaveFile(AppendFileName(folder, "report.txt"), Format("PASS: %d style/optical/fill combinations; 48/64 rendering, legacy artwork, saved identity, undo, PNG/SVG and paired IML export.\n", variant_count));
	return true;
}

}

