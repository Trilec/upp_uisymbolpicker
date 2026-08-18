#include "SymbolPickerFileActions.h"

#include "SymbolPickerProjectIo.h"
#include "SymbolPickerUppExport.h"
#include "SymbolPickerImlExport.h"

namespace Upp {

namespace {

static const char *kProjectExtension = ".uppicons.json";

String StripProjectSuffixes(String path)
{
	String lower = ToLower(path);
	if(lower.EndsWith(".json")) {
		path.Trim(path.GetCount() - 5);
		lower = ToLower(path);
	}
	while(lower.EndsWith(".uppicons")) {
		path.Trim(path.GetCount() - 9);
		lower = ToLower(path);
	}
	return path;
}

String EnsureProjectExtension(String path)
{
	return StripProjectSuffixes(path) + kProjectExtension;
}

String StripProjectExtension(String name)
{
	return StripProjectSuffixes(name);
}

String EnsureFileExtension(String path, const String& ext)
{
	if(!ToLower(path).EndsWith(ToLower(ext)))
		path << ext;
	return path;
}

String PreferredDialogDir(const String& current_path)
{
	String dir = current_path.IsEmpty() ? GetCurrentDirectory() : GetFileFolder(current_path);
	if(!DirectoryExists(dir))
		dir = GetCurrentDirectory();
	return dir;
}

String MakeExportScopeName(const SymbolPickerProject& project, SymbolPickerExportScope scope)
{
	String base = TrimBoth(project.output_base_name);
	if(base.IsEmpty())
		base = TrimBoth(project.project_name);
	if(base.IsEmpty())
		base = "symbolpicker_export";
	base = MakeSymbolPickerSafeCppIdentifierSegment(base);
	base << (scope == SymbolPickerExportScope::AllCollections ? "_all" : "_current");
	return ToLower(base);
}

String MakeExportTypeName(SymbolPickerExportType type)
{
	switch(type) {
	case SymbolPickerExportType::CppSnippet: return "cpp";
	case SymbolPickerExportType::UppRawHeader: return "raw";
	case SymbolPickerExportType::UppRleHeader: return "rle";
	case SymbolPickerExportType::UppIml: return "iml";
	case SymbolPickerExportType::UppImlLibrary: return "iml_library";
	case SymbolPickerExportType::PngFiles: return "png";
	case SymbolPickerExportType::SvgFiles: return "svg";
	case SymbolPickerExportType::IconId: return "icon_id";
	case SymbolPickerExportType::ImageCall:
	default: return "image_call";
	}
}

String WarningDetail(const Vector<String>& warnings, const String& folder)
{
	if(warnings.IsEmpty())
		return String();
	String warnings_path = AppendFileName(folder, "_export_warnings.txt");
	String text;
	for(const String& warning : warnings)
		text << warning << '\n';
	if(!SaveFile(warnings_path, text))
		return Format("Warnings: %d", warnings.GetCount());
	return Format("Warnings: %d (see %s)", warnings.GetCount(), GetFileName(warnings_path));
}

} // namespace

void SymbolPickerFileActions::SetContext(SymbolPickerModel* model,
	const SymbolPickerCatalog* catalog,
	SymbolPickerCommandStack* commands)
{
	model_ = model;
	catalog_ = catalog;
	commands_ = commands;
}

String SymbolPickerFileActions::BuildProjectDialogTitle(const char* verb) const
{
	String label = model_ ? model_->GetProjectName() : String();
	if(label.IsEmpty())
		label = "SymbolPicker Project";
	return Format("%s %s", verb, label);
}

String SymbolPickerFileActions::MakeExportDefaultExtension() const
{
	if(!model_)
		return ".txt";
	switch(model_->GetExportType()) {
	case SymbolPickerExportType::CppSnippet: return ".cpp";
	case SymbolPickerExportType::UppRawHeader:
	case SymbolPickerExportType::UppRleHeader:
	case SymbolPickerExportType::UppImlLibrary: return ".h";
	case SymbolPickerExportType::UppIml: return ".iml";
	case SymbolPickerExportType::PngFiles:
	case SymbolPickerExportType::SvgFiles:
	case SymbolPickerExportType::IconId:
	case SymbolPickerExportType::ImageCall:
	default: return ".txt";
	}
}

String SymbolPickerFileActions::MakeExportDefaultName(SymbolPickerExportScope scope) const
{
	if(!model_)
		return "symbolpicker_export";
	String base = MakeExportScopeName(model_->ExportProject(), scope);
	switch(model_->GetExportType()) {
	case SymbolPickerExportType::UppRawHeader: base << "_raw"; break;
	case SymbolPickerExportType::UppRleHeader: base << "_rle"; break;
	case SymbolPickerExportType::UppIml: base << "_iml"; break;
	case SymbolPickerExportType::UppImlLibrary: return "UiIcons";
	default: base << '_' << MakeExportTypeName(model_->GetExportType()); break;
	}
	return base;
}

String SymbolPickerFileActions::BuildExportText(SymbolPickerExportScope scope, Vector<String>* warnings) const
{
	if(!model_ || !catalog_)
		return String();

	SymbolPickerProject project = model_->ExportProject();
	project.default_size = model_->GetExportSize();
	Vector<String> local_warnings;
	Vector<String>& warn = warnings ? *warnings : local_warnings;

	switch(model_->GetExportType()) {
	case SymbolPickerExportType::IconId: return BuildIconIdExport(project, *catalog_, scope, &warn);
	case SymbolPickerExportType::CppSnippet: return BuildCppSnippetExport(project, *catalog_, scope, &warn);
	case SymbolPickerExportType::UppRawHeader: return BuildSymbolPickerUppRawHeader(project, *catalog_, scope, &warn);
	case SymbolPickerExportType::UppRleHeader: return BuildSymbolPickerUppRleHeader(project, *catalog_, scope, &warn);
	case SymbolPickerExportType::UppIml: return BuildSymbolPickerUppIml(project, *catalog_, scope, &warn);
	case SymbolPickerExportType::UppImlLibrary:
	case SymbolPickerExportType::PngFiles:
	case SymbolPickerExportType::SvgFiles: return String();
	case SymbolPickerExportType::ImageCall:
	default: return BuildImageCallExport(project, *catalog_, scope, &warn);
	}
}

void SymbolPickerFileActions::ValidateLoadedProject(SymbolPickerProject& project) const
{
	if(project.collections.IsEmpty()) {
		SymbolPickerCollection collection;
		collection.name = "Collection 1";
		collection.file_path = project.file_path;
		project.collections.Add(pick(collection));
		project.active_collection_index = 0;
		return;
	}

	for(auto& collection : project.collections) {
		collection.file_path = project.file_path;
		collection.dirty = false;
		for(auto& item : collection.items)
			item.unresolved = !catalog_ || !catalog_->FindByCatalogId(item.catalog_id);
	}
	if(project.active_collection_index < 0 || project.active_collection_index >= project.collections.GetCount())
		project.active_collection_index = 0;
}

void SymbolPickerFileActions::ShowExportComplete(const String& file_name, const String& folder, const String& detail) const
{
	String message = "[*+110 " + DeQtf(file_name) + "]&[+85 " + DeQtf(folder) + "]";
	if(!detail.IsEmpty())
		message << "&[+80 " << DeQtf(detail) << "]";
	PromptOK(message);
}

bool SymbolPickerFileActions::SaveProject(bool save_as)
{
	if(!model_) {
		Exclamation("No SymbolPicker model is attached.");
		return false;
	}

	String path = model_->GetProjectFilePath();
	if(save_as || path.IsEmpty()) {
		FileSel fs;
		fs.Type("SymbolPicker project", "*.uppicons.json");
		fs.DefaultExt("uppicons.json");
		String dir = PreferredDialogDir(path);
		fs.ActiveDir(dir);
		String base_name = model_->GetProjectName();
		if(base_name.IsEmpty())
			base_name = "symbolpicker_project";
		fs.PreSelect(AppendFileName(dir, StripProjectExtension(base_name)));
		if(!fs.ExecuteSaveAs(BuildProjectDialogTitle("Save")))
			return false;
		path = EnsureProjectExtension(~fs);
	}
	else
		path = EnsureProjectExtension(path);

	SymbolPickerProject project = model_->ExportProject();
	if(project.project_name.IsEmpty())
		project.project_name = GetFileTitle(path);
	project.file_path = path;
	for(auto& collection : project.collections)
		collection.file_path = path;

	String error;
	if(!SaveSymbolPickerProjectJson(project, path, error)) {
		Exclamation(error);
		return false;
	}
	model_->SetProjectFilePath(path);
	if(model_->GetProjectName().IsEmpty())
		model_->SetProjectName(project.project_name);
	model_->MarkCollectionsSaved();
	PromptOK(Format("Saved project:\n%s", path));
	return true;
}

bool SymbolPickerFileActions::LoadProject()
{
	if(!model_ || !commands_) {
		Exclamation("SymbolPicker is not fully wired.");
		return false;
	}

	FileSel fs;
	fs.Type("SymbolPicker project", "*.uppicons.json");
	fs.ActiveDir(PreferredDialogDir(model_->GetProjectFilePath()));
	if(!fs.ExecuteOpen(BuildProjectDialogTitle("Load")))
		return false;

	String path = ~fs;
	SymbolPickerProject project;
	String error;
	if(!LoadSymbolPickerProjectJson(path, project, error)) {
		Exclamation(error);
		return false;
	}
	if(project.project_name.IsEmpty())
		project.project_name = GetFileTitle(path);
	project.file_path = path;
	ValidateLoadedProject(project);
	if(!model_->LoadProject(project))
		return false;
	commands_->Clear();
	PromptOK(Format("Loaded project:\n%s", path));
	return true;
}

bool SymbolPickerFileActions::CopyCurrentExportToClipboard()
{
	if(!model_ || !catalog_) {
		Exclamation("SymbolPicker is not fully wired.");
		return false;
	}
	if(model_->GetExportType() == SymbolPickerExportType::SvgFiles ||
	   model_->GetExportType() == SymbolPickerExportType::PngFiles ||
	   model_->GetExportType() == SymbolPickerExportType::UppImlLibrary) {
		PromptOK("This export type writes files and uses Export.");
		return false;
	}
	const SymbolPickerCollection* collection = model_->GetActiveCollection();
	if(!collection || collection->items.IsEmpty()) {
		PromptOK("No items in the active collection.");
		return false;
	}

	Vector<String> warnings;
	String text = BuildExportText(SymbolPickerExportScope::ActiveCollection, &warnings);
	if(text.IsEmpty()) {
		PromptOK("Nothing exportable in the active collection.");
		return false;
	}
	WriteClipboardText(text);
	PromptOK("Export text copied to clipboard.");
	return true;
}

bool SymbolPickerFileActions::Export(SymbolPickerExportScope scope)
{
	if(!model_ || !catalog_) {
		Exclamation("SymbolPicker is not fully wired.");
		return false;
	}
	switch(model_->GetExportType()) {
	case SymbolPickerExportType::SvgFiles: return ExportSvgFiles(scope);
	case SymbolPickerExportType::PngFiles: return ExportPngFiles(scope);
	case SymbolPickerExportType::UppImlLibrary: return ExportImlLibrary(scope);
	default: return ExportText(scope);
	}
}

bool SymbolPickerFileActions::ExportText(SymbolPickerExportScope scope)
{
	Vector<String> warnings;
	String text = BuildExportText(scope, &warnings);
	if(text.IsEmpty()) {
		PromptOK(scope == SymbolPickerExportScope::AllCollections
			? "No exportable items in all collections."
			: "No exportable items in the active collection.");
		return false;
	}

	FileSel fs;
	String ext = MakeExportDefaultExtension();
	String type_name = "Text file";
	String type_filter = "*.txt";
	switch(model_->GetExportType()) {
	case SymbolPickerExportType::CppSnippet: type_name = "C++ source"; type_filter = "*.cpp"; break;
	case SymbolPickerExportType::UppRawHeader:
	case SymbolPickerExportType::UppRleHeader: type_name = "U++ header"; type_filter = "*.h"; break;
	case SymbolPickerExportType::UppIml: type_name = "U++ IML"; type_filter = "*.iml"; break;
	default: break;
	}
	fs.Type(type_name, type_filter);
	fs.DefaultExt(ext.Mid(1));
	String dir = PreferredDialogDir(model_->GetProjectFilePath());
	fs.ActiveDir(dir);
	fs.PreSelect(AppendFileName(dir, MakeExportDefaultName(scope) + ext));
	if(!fs.ExecuteSaveAs(BuildProjectDialogTitle("Export")))
		return false;

	String path = EnsureFileExtension(~fs, ext);
	if(!SaveFile(path, text)) {
		Exclamation(Format("Could not write export file:\n%s", path));
		return false;
	}
	ShowExportComplete(GetFileName(path), GetFileFolder(path), WarningDetail(warnings, GetFileFolder(path)));
	return true;
}

bool SymbolPickerFileActions::ExportImlLibrary(SymbolPickerExportScope scope)
{
	FileSel fs;
	fs.Type("U++ IML library header", "*.h");
	fs.DefaultExt("h");
	String dir = PreferredDialogDir(model_->GetProjectFilePath());
	fs.ActiveDir(dir);
	fs.PreSelect(AppendFileName(dir, MakeExportDefaultName(scope) + ".h"));
	if(!fs.ExecuteSaveAs(BuildProjectDialogTitle("Export IML Library")))
		return false;

	String header_path = EnsureFileExtension(~fs, ".h");
	String base = GetFileTitle(header_path);
	String iml_path = AppendFileName(GetFileFolder(header_path), base + ".iml");
	SymbolPickerProject project = model_->ExportProject();
	project.default_size = model_->GetExportSize();
	Vector<String> warnings;
	String iml_text, header_text;
	if(!BuildSymbolPickerUppImlLibrary(project, *catalog_, scope, GetFileName(iml_path), iml_text, header_text, &warnings)) {
		PromptOK("No exportable items for the IML library.");
		return false;
	}
	if(!SaveFile(iml_path, iml_text)) {
		Exclamation(Format("Could not write IML file:\n%s", iml_path));
		return false;
	}
	if(!SaveFile(header_path, header_text)) {
		FileDelete(iml_path);
		Exclamation(Format("Could not write library header:\n%s", header_path));
		return false;
	}
	ShowExportComplete(GetFileName(header_path) + " + " + GetFileName(iml_path),
		GetFileFolder(header_path), WarningDetail(warnings, GetFileFolder(header_path)));
	return true;
}

bool SymbolPickerFileActions::ExportSvgFiles(SymbolPickerExportScope scope)
{
	FileSel fs;
	fs.Type("Folder", ".*");
	String dir = PreferredDialogDir(model_->GetProjectFilePath());
	fs.ActiveDir(dir);
	if(!fs.ExecuteSelectDir(BuildProjectDialogTitle(scope == SymbolPickerExportScope::AllCollections ? "Export All SVG" : "Export SVG")))
		return false;

	String output_folder = ~fs;
	Vector<String> warnings;
	int written = 0, skipped = 0;
	SymbolPickerProject project = model_->ExportProject();
	project.default_size = model_->GetExportSize();
	if(!ExportSymbolPickerSvgFiles(project, *catalog_, scope, output_folder, &warnings, &written, &skipped)) {
		Exclamation(Format("SVG export finished with issues.\nWritten: %d\nSkipped: %d\nPath: %s", written, skipped, output_folder));
		return false;
	}
	String detail = Format("Written: %d | Skipped: %d", written, skipped);
	String warning_detail = WarningDetail(warnings, output_folder);
	if(!warning_detail.IsEmpty())
		detail << " | " << warning_detail;
	ShowExportComplete(Format("%d SVG file%s", written, written == 1 ? "" : "s"), output_folder, detail);
	return true;
}

bool SymbolPickerFileActions::ExportPngFiles(SymbolPickerExportScope scope)
{
	FileSel fs;
	fs.Type("Folder", ".*");
	String dir = PreferredDialogDir(model_->GetProjectFilePath());
	fs.ActiveDir(dir);
	if(!fs.ExecuteSelectDir(BuildProjectDialogTitle(scope == SymbolPickerExportScope::AllCollections ? "Export All PNG" : "Export PNG")))
		return false;

	String output_folder = ~fs;
	Vector<String> warnings;
	int written = 0, skipped = 0;
	SymbolPickerProject project = model_->ExportProject();
	project.default_size = model_->GetExportSize();
	if(!ExportSymbolPickerPngFiles(project, *catalog_, scope, output_folder, &warnings, &written, &skipped)) {
		Exclamation(Format("PNG export finished with issues.\nWritten: %d\nSkipped: %d\nPath: %s", written, skipped, output_folder));
		return false;
	}
	String detail = Format("Written: %d | Skipped: %d", written, skipped);
	String warning_detail = WarningDetail(warnings, output_folder);
	if(!warning_detail.IsEmpty())
		detail << " | " << warning_detail;
	ShowExportComplete(Format("%d PNG file%s", written, written == 1 ? "" : "s"), output_folder, detail);
	return true;
}

} // namespace Upp
