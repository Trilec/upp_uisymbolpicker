#include "SymbolPickerApp.h"
#include "SymbolPickerImlExport.h"
#include "SymbolPickerCustomAssets.h"

using namespace Upp;

GUI_APP_MAIN
{
	SymbolPickerApp app;
	String error;
	const Vector<String>& args = CommandLine();
	if(args.GetCount() >= 3 && (args[0] == "--compare-material" || args[0] == "--scan-usage" || args[0] == "--download-material" || args[0] == "--install-refresh")) {
		SymbolPickerMaintenanceJob job;
		SymbolPickerMaintenanceResult result;
		String output;
		if(args[0] == "--download-material" && args.GetCount() == 3) {
			output = args[2]; DownloadSymbolPickerSources(args[1], output, job, result);
		}
		else if(args.GetCount() >= 4) {
			output = args[3];
			if(args[0] == "--compare-material") {
				Vector<SymbolPickerSourceRow> rows; GetSymbolPickerSourceRows(rows);
				CompareSymbolPickerSources(rows, args[1], args[2] == "-" ? String() : args[2], output, args.GetCount() == 5 && args[4] == "--prepare", job, result);
			}
			else if(args[0] == "--scan-usage") {
				ScanSymbolPickerUsage(args[1], args[2], output, job, result);
				if(result.ok && args.GetCount() == 5 && args[4] == "--apply") {
					SymbolPickerMaintenanceResult applied; ApplySymbolPickerUsage(result, output, job, applied); result = applied;
				}
			}
			else if(args[0] == "--install-refresh" && args.GetCount() == 4) InstallSymbolPickerRefresh(args[1], args[2], job, result);
		}
		if(!output.IsEmpty()) {
			RealizeDirectory(output); result.report.Set("ok", result.ok); result.report.Set("summary", result.summary); result.report.Set("error", result.error);
			SaveFile(AppendFileName(output, "last-result.json"), AsJSON(result.report, true));
		}
		SetExitCode(result.ok ? 0 : 1); return;
	}
	bool self_test = args.GetCount() > 0 && args[0] == "--self-test";
	bool headless = self_test || (args.GetCount() > 0 && (args[0] == "--seed-ui-project" || args[0] == "--export-project"));
	if(!app.Init(error, self_test)) {
		if(headless) {
			if(self_test && args.GetCount() > 1) SaveFile(args[1], "FAIL: " + error + "\n");
			SetExitCode(1);
		}
		else
			Exclamation(error.IsEmpty() ? "SymbolPicker startup smoke tests failed." : error);
		return;
	}
	if(self_test) {
		bool passed = app.GetView().RunSearchSmokeTests(error);
		if(args.GetCount() > 1)
			SaveFile(args[1], passed ? "PASS: native maintenance/utility themes, startup, full catalogue/gallery scale, search callbacks, zoom, theme transitions, project and export smoke tests.\n" : "FAIL: " + error + "\n");
		SetExitCode(passed ? 0 : 1);
		return;
	}
	if(args.GetCount() == 2 && args[0] == "--seed-ui-project") {
		SymbolPickerProject project;
		project.project_name = "UiIcons";
		project.output_base_name = "UiIcons";
		project.symbol_prefix = "ICON_";
		project.default_size = 48;
		project.active_collection_index = 0;
		project.comment = "Inventory of the linked shared Ui catalogue. Existing names and original artwork are preserved. Review generated integration before replacing shared assets.";
		VectorMap<String, int> groups;
		for(const auto& icon : UiIconCatalog()) {
			String name = icon.name.StartsWith("ICON_") ? icon.name.Mid(5) : icon.name;
			int separator = name.Find('_');
			String category = separator >= 0 ? name.Left(separator) : String("OTHER");
			int group = groups.Find(category);
			if(group < 0) { group = groups.GetCount(); groups.Add(category, group); project.collections.Add().name = category; }
			SymbolPickerIconRef& ref = project.collections[group].items.Add();
			ref.catalog_id = "ui/" + icon.name;
			ref.source_id = ref.catalog_id;
			ref.alias = icon.name;
			ref.size = 48;
			Size original = icon.factory().GetSize();
			ref.comment = Format("Existing Ui asset; native dimensions %d x %d. Export canvas 48x48. Raster enlargement does not add source detail.", original.cx, original.cy);
		}
		SetExitCode(SaveSymbolPickerProjectJson(project, args[1], error) ? 0 : 1);
		return;
	}
	if(args.GetCount() >= 2 && (args[0] == "--project" || args[0] == "--export-project")) {
		SymbolPickerProject project;
		if(!LoadSymbolPickerProjectJson(args[1], project, error)) { if(!headless) Exclamation(error); SetExitCode(1); return; }
		app.GetCatalog().SetCustomAssets(project.custom_assets);
		app.GetView().SetCatalog(&app.GetCatalog());
		for(auto& collection : project.collections)
			for(auto& item : collection.items)
				item.unresolved = !app.GetCatalog().FindByCatalogId(item.catalog_id) || !app.GetCatalog().FindByCatalogId(item.catalog_id)->available;
		if(args[0] == "--export-project") {
			if(args.GetCount() != 3) { SetExitCode(1); return; }
			String target = ForceExt(args[2], ".h");
			String iml_path = ForceExt(target, ".iml");
			String iml, header;
			Vector<String> warnings;
			bool ok = BuildSymbolPickerUppImlLibrary(project, app.GetCatalog(), SymbolPickerExportScope::AllCollections,
				iml_path, iml, header, &warnings);
			String warning_text;
			for(const auto& warning : warnings) warning_text << warning << '\n';
			if(!warning_text.IsEmpty()) SaveFile(ForceExt(target, ".warnings.txt"), warning_text);
			SetExitCode(ok && SaveFile(iml_path, iml) && SaveFile(target, header) && warnings.IsEmpty() ? 0 : 1);
			return;
		}
		app.GetModel().LoadProject(project);
		app.GetModel().SetProjectFilePath(args[1]);
		app.GetModel().MarkCollectionsSaved();
		app.GetCommands().Clear();
	}
	app.Run();
}
