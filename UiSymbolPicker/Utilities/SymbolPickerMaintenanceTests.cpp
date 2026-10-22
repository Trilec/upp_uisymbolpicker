#include "SymbolPickerMaintenance.h"
#include <plugin/z/z.h>

namespace Upp {

bool RunSymbolPickerMaintenanceTests(String& error)
{
	try {
		String folder = AppendFileName(GetExeFolder(), "maintenance-tests-" + FormatInt64((int64)Random64()));
		RealizeDirectory(folder);
		auto Put = [&](const String& path, const String& text) { RealizeDirectory(GetFileFolder(path)); if(!SaveFile(path, text)) throw Exc("Cannot write maintenance fixture."); };
		auto Require = [&](bool condition, const char* message) { if(!condition) throw Exc(message); };
		String source = AppendFileName(folder, "download");
		const char* styles[] = { "materialsymbolsoutlined", "materialsymbolsrounded", "materialsymbolssharp" };
		String svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\"><path d=\"M0 0h24v24z\"/></svg>";
		String pretty = "<svg viewBox=\"0 0 24 24\" xmlns=\"http://www.w3.org/2000/svg\">\n  <path d=\"M0 0h24v24z\"/>\n</svg>";
		Vector<SymbolPickerSourceRow> current;
		for(int i = 0; i < 3; ++i) {
			String path = "symbols/web/old/" + String(styles[i]) + "/old_24px.svg";
			Put(AppendFileName(source, path), i == 0 ? pretty : svg);
			auto& row = current.Add(); row.category = "action"; row.name = "old"; row.style = i; row.source = path; row.packed_svg = Base64Encode(ZCompress(svg));
			Put(AppendFileName(source, "symbols/web/new/" + String(styles[i]) + "/new_24px.svg"), svg);
		}
		auto& retired = current.Add(); retired.category = "action"; retired.name = "retired"; retired.style = 0; retired.source = "retired.svg"; retired.packed_svg = Base64Encode(ZCompress(svg));
		String metadata = AppendFileName(folder, "metadata.json");
		Put(metadata, "{\"icons\":[{\"name\":\"old\",\"categories\":[\"Actions\"]},{\"name\":\"new\",\"tags\":[\"grip\",\"rearrange\"],\"categories\":[\"Activities\",\"Business\"]}]}" );
		Put(AppendFileName(source, "LICENSE"), "fixture license");
		SymbolPickerMaintenanceJob job; SymbolPickerMaintenanceResult compare;
		CompareSymbolPickerSources(current, source, metadata, folder, true, job, compare);
		Require(compare.ok, ~compare.error);
		Require(ValueArray(compare.report["changed_artwork"]).GetCount() == 0, "Whitespace/attribute order produced a false artwork change.");
		Require(ValueArray(compare.report["missing_variants"]).GetCount() == 3, "Missing variants were counted incorrectly.");
		Require(ValueArray(compare.report["removed_upstream_variants_retained"]).GetCount() == 1, "Retired artwork was not reported.");
		Require((int)compare.report["staged_category_rows"] == 10, "New memberships or retained IDs were lost.");
		Require(LoadFile(AppendFileName(compare.stage, "SearchTags.h")).Find("grip rearrange") >= 0, "Refresh did not preserve Google search tags.");
		Require(FileExists(AppendFileName(compare.stage, "icons_activitie.h")) && FileExists(AppendFileName(compare.stage, "icons_busines.h")), "Historical category naming changed.");
		Put(AppendFileName(source, "symbols/web/old/" + String(styles[1]) + "/old_24px.svg"), "<svg><path d=\"M1 1h10v10z\"/></svg>");
		SymbolPickerMaintenanceResult changed; CompareSymbolPickerSources(current, source, metadata, folder, false, job, changed);
		Require(changed.ok && ValueArray(changed.report["changed_artwork"]).GetCount() == 1, "A real artwork change was missed.");
		FileDelete(AppendFileName(source, "symbols/web/new/" + String(styles[2]) + "/new_24px.svg"));
		SymbolPickerMaintenanceResult incomplete; CompareSymbolPickerSources(current, source, metadata, folder, true, job, incomplete);
		Require(!incomplete.ok && incomplete.stage.IsEmpty() && !ValueArray(incomplete.report["incomplete_upstream_styles"]).IsEmpty(), "Incomplete styles allowed staging.");
		String picker = AppendFileName(folder, "picker"); Put(AppendFileName(picker, "UiSymbolPicker/UiSymbolPicker.upp"), "uses Core;\nfile\n\tGenerated/CatalogTables.h,\n\tmain.cpp;\n"); Put(AppendFileName(picker, "UiSymbolPicker/Generated/CatalogTables.h"), "old manifest");
		SymbolPickerMaintenanceResult installed; InstallSymbolPickerRefresh(compare.stage, picker, job, installed);
		Require(installed.ok, ~installed.error);
		String backup = AsString(installed.report["backup"]);
		Require(LoadFile(AppendFileName(backup, "CatalogTables.h")) == "old manifest", "Install did not preserve the original manifest.");
		Require(LoadFile(AppendFileName(picker, "UiSymbolPicker/UiSymbolPicker.upp")).Find("Generated/icons_activitie.h") >= 0, "Install did not register new category headers in the package.");
		Put(AppendFileName(compare.stage, "icons_action.h"), "modified staged data");
		SymbolPickerMaintenanceResult tampered; InstallSymbolPickerRefresh(compare.stage, picker, job, tampered);
		Require(!tampered.ok, "A modified stage was installed without re-preparing.");
		String project = AppendFileName(folder, "AppIcons.symbolpicker"), code = AppendFileName(folder, "code");
		Put(project, "{\"output_base_name\":\"AppIcons\",\"custom_field\":42,\"collections\":[{\"name\":\"Main\",\"items\":[{\"alias\":\"APP_SAVE\",\"catalog_id\":\"action/save/outlined\",\"usage_note\":\"Manual note\"},{\"alias\":\"APP_UNUSED\",\"marked_used\":true,\"usage_note\":\"Keep this warning\"}]}]}" );
		Put(AppendFileName(code, "caller.cpp"), "APP_SAVE(); // APP_SAVE in comment\nAPP_SAVE_EXTRA(); 9APP_UNUSED;");
		Put(AppendFileName(code, "AppIcons.h"), "APP_UNUSED"); Put(AppendFileName(code, "Generated/table.h"), "APP_UNUSED");
		SymbolPickerMaintenanceResult scan; ScanSymbolPickerUsage(project, code, folder, job, scan);
		Require(scan.ok, ~scan.error);
		Require((int)scan.report["matched_items"] == 1, "Identifier boundaries or definition exclusions failed.");
		ValueMap modified(ParseJSON(scan.project_after)); ValueArray saved_groups(modified["collections"]); ValueMap group(saved_groups[0]); ValueArray items(group["items"]);
		Require((bool)ValueMap(items[0])["marked_used"] && AsString(ValueMap(items[0])["usage_note"]).StartsWith("Manual note\nScan evidence:"), "Usage scan lost a manual note.");
		Require((bool)ValueMap(items[1])["marked_used"] && AsString(ValueMap(items[1])["usage_note"]) == "Keep this warning" && (int)modified["custom_field"] == 42, "Scan cleared marks or extra project metadata.");
		SymbolPickerMaintenanceResult applied; ApplySymbolPickerUsage(scan, folder, job, applied);
		Require(applied.ok && LoadFile(project) == scan.project_after, ~applied.error);
		Require(LoadFile(AsString(applied.report["backup"])) == scan.project_before, "Usage backup differs from the original.");
		SymbolPickerMaintenanceResult stale; ApplySymbolPickerUsage(scan, folder, job, stale); Require(!stale.ok, "Stale scan overwrote a changed project.");
		job.cancelled = true; SymbolPickerMaintenanceResult cancelled; CompareSymbolPickerSources(current, source, metadata, folder, true, job, cancelled); Require(!cancelled.ok && cancelled.stage.IsEmpty(), "Cancellation allowed a refresh.");
		return true;
	}
	catch(const Exc& exc) { error = "Maintenance: " + String(exc); return false; }
}
}
