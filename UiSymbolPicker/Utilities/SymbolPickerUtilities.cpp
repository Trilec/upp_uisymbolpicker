#include "SymbolPickerUtilities.h"
#include "SymbolPickerMaintenance.h"
#include "../SymbolPickerGeneratedCatalog.h"

namespace Upp {
namespace {

class UtilityDocument : public UiDoc {
public:
	bool Key(dword key, int count) override {
		if(key == K_CTRL_C || key == K_CTRL_A) return UiDoc::Key(key, count);
		dword nav = key & ~(K_CTRL | K_SHIFT);
		if(nav == K_LEFT || nav == K_RIGHT || nav == K_UP || nav == K_DOWN || nav == K_HOME || nav == K_END || nav == K_PAGEUP || nav == K_PAGEDOWN) return UiDoc::Key(key, count);
		if(key == K_TAB || key == (K_SHIFT | K_TAB) || key == K_ESCAPE) return false;
		return true;
	}
};
struct FormRow {
	UiBoxLayout box { UiDirection::H };
	UiLabel label;
	UiLineEdit edit;
	UiButton browse;
	void Build(UiBoxLayout& parent, const char* text, bool file = false) {
		box.SetGap(DPI(8)).SetAlignItems(UiCrossAlign::Center);
		label.SetText(text); browse.SetText("Browse…");
		box.Add(label).Fixed(DPI(150)); box.Add(edit).Expand(1); box.Add(browse).Fixed(DPI(82));
		parent.Add(box).Fixed(DPI(32));
		browse.WhenAction = [this, file] {
			FileSel selector;
			if(file) { selector.Type("Symbol Picker / metadata", "*.symbolpicker *.json"); selector.AllFilesType(); }
			selector <<= AsString(edit.GetData());
			if(file ? selector.ExecuteOpen() : selector.ExecuteSelectDir()) edit.SetData(~selector);
			edit.WhenAction();
		};
	}
	String Text() const { return TrimBoth(AsString(edit.GetData())); }
};

String LocateSource() {
	String folder = GetExeFolder();
	for(int i = 0; i < 6; ++i) {
		if(FileExists(AppendFileName(folder, "UiSymbolPicker/UiSymbolPicker.upp"))) return folder;
		String parent = GetFileFolder(folder); if(parent == folder) break; folder = parent;
	}
	return String();
}

class UtilitiesWindow : public TopWindow {
public:
	UtilitiesWindow(const String& project_path) {
		Ctrl::SkinChangeSensitive();
		Title("Symbol Picker — Utilities").Sizeable().Zoomable();
		SetRect(0, 0, DPI(1000), DPI(820)); SetMinSize(Size(DPI(780), DPI(700)));
		root_.SetGap(DPI(10)).SetInset(DPI(16)); Add(root_.SizePos());
		title_.SetTitle("Symbol library utilities").SetSubTitle("Keep your catalogue current and find where your application uses its icons")
		      .SetSelectable(false).EnableHover(false);
		root_.Add(title_).Fit();
		mode_label_.SetText("Choose a tool");
		mode_.Add("Catalogue updates", 0); mode_.Add("Check icon usage", 1); mode_.SetData(0);
		help_.SetText("Help");
		mode_row_.SetGap(DPI(12)).SetAlignItems(UiCrossAlign::Center);
		mode_row_.Add(mode_label_).Fit(); mode_row_.Add(mode_).Fixed(DPI(240)); mode_row_.AddSpacer(1).Expand(1); mode_row_.Add(help_).Fixed(DPI(72));
		root_.Add(mode_row_).Fixed(DPI(34));
		catalogue_panel_.Add(catalogue_form_.SizePos()); usage_panel_.Add(usage_form_.SizePos());
		catalogue_form_.SetGap(DPI(8)).SetInset(DPI(12)); usage_form_.SetGap(DPI(8)).SetInset(DPI(12));
		catalogue_hint_.SetText("1. Choose an existing download, or download a fresh snapshot.    2. Compare.    3. Prepare and install.");
		catalogue_form_.Add(catalogue_hint_).Fit();
		remote_.Build(catalogue_form_, "Download from"); remote_.box.RemoveItem(2);
		remote_.edit.SetData("https://github.com/Trilec/material-design-icons.git"); remote_.edit.Tip("An HTTPS Git repository. Downloads create a new folder; existing checkouts are left in place.");
		source_.Build(catalogue_form_, "Material download"); source_.edit.Tip("The repository root containing symbols/web. Choose the whole material-design-icons folder.");
		metadata_.Build(catalogue_form_, "Category metadata", true); metadata_.edit.Tip("Optional for comparison; required to prepare new category entries. Download latest fetches this JSON too.");
		picker_.Build(catalogue_form_, "Picker source folder"); picker_.edit.Tip("The source folder containing UiSymbolPicker/UiSymbolPicker.upp. Needed only to install a prepared refresh.");
		String repo = LocateSource(); picker_.edit.SetData(repo);
		String existing = AppendFileName(repo, "build/material-design-icons-upstream");
		if(DirectoryExists(existing)) source_.edit.SetData(existing);
		String meta = AppendFileName(repo, "build/material-symbols-metadata.json"); if(FileExists(meta)) metadata_.edit.SetData(meta);
		download_.SetText("Download latest"); compare_.SetText("Compare"); prepare_.SetText("Prepare refresh"); install_.SetText("Install prepared refresh");
		catalogue_actions_.SetGap(DPI(10)); catalogue_actions_.Add(download_).Fit(); catalogue_actions_.Add(compare_).Fit(); catalogue_actions_.Add(prepare_).Fit(); catalogue_actions_.AddSpacer(1).Expand(1); catalogue_actions_.Add(install_).Fit();
		catalogue_form_.Add(catalogue_actions_).Fixed(DPI(34));
		usage_hint_.SetText("1. Save your collection project.    2. Choose your application's code folder.    3. Scan and review.");
		usage_form_.Add(usage_hint_).Fit();
		project_.Build(usage_form_, "Saved project", true); project_.edit.SetData(project_path);
		code_.Build(usage_form_, "Application code");
		if(!project_path.IsEmpty()) code_.edit.SetData(GetFileFolder(project_path));
		usage_note_.SetText("Matches add warnings only. Existing marks stay in place. Save open edits before applying, then reload the project.");
		usage_form_.Add(usage_note_).Fit();
		scan_.SetText("Scan usage"); apply_.SetText("Apply usage warnings");
		usage_actions_.SetGap(DPI(10)); usage_actions_.Add(scan_).Fit(); usage_actions_.AddSpacer(1).Expand(1); usage_actions_.Add(apply_).Fit(); usage_form_.Add(usage_actions_).Fixed(DPI(34));
		pages_.AddPage(catalogue_panel_, "catalogue"); pages_.AddPage(usage_panel_, "usage"); pages_.SetActivePage(0);
		root_.Add(pages_).Fit();
		output_.Build(root_, "Reports and backups"); output_.edit.SetData(repo.IsEmpty() ? AppendFileName(GetHomeDirectory(), "SymbolPicker-maintenance") : AppendFileName(repo, "build/maintenance"));
		root_.Add(document_).Expand(1).MinMain(DPI(160)); document_.ShowMetadataMarkers(false);
		status_.SetText("Ready. Compare and scan leave your files unchanged."); cancel_.SetText("Cancel job"); close_.SetText("Close"); copy_.SetText("Copy report");
		footer_.SetGap(DPI(10)).SetAlignItems(UiCrossAlign::Center); footer_.Add(status_).Expand(1); footer_.Add(copy_).Fit(); footer_.Add(cancel_).Fit(); footer_.Add(close_).Fit(); root_.Add(footer_).Fixed(DPI(34));
		mode_.WhenAction = [=] { pages_.SetActivePage((int)mode_.GetData()); ShowHelp(); UpdateActions(); };
		mode_.WhenSelectData = [=](const Value&) { mode_.WhenAction(); };
		help_.WhenAction = [=] { ShowHelp(); };
		copy_.WhenAction = [=] { WriteClipboardText(AsJSON(result_.report, true)); };
		cancel_.WhenAction = [=] { job_.cancelled = true; status_.SetText("Cancelling…"); };
		close_.WhenAction = [=] { RequestClose(); }; WhenClose = [=] { RequestClose(); };
		download_.WhenAction = [=] { Start(Download); }; compare_.WhenAction = [=] { Start(Compare); }; prepare_.WhenAction = [=] { Start(Prepare); }; install_.WhenAction = [=] { Start(Install); }; scan_.WhenAction = [=] { Start(Scan); }; apply_.WhenAction = [=] { Start(Apply); };
		auto invalidate = [=] { prepared_.Clear(); scan_result_ = SymbolPickerMaintenanceResult(); UpdateActions(); };
		for(FormRow* row : { &remote_, &source_, &metadata_, &picker_, &project_, &code_, &output_ }) {
			row->edit.WhenAction = invalidate; row->edit.WhenChange = invalidate;
		}
		ApplyTheme(); ShowHelp(); UpdateActions();
	}
	~UtilitiesWindow() { job_.cancelled = true; KillTimeCallback(TIMEID_POLL); if(worker_.IsOpen()) worker_.Wait(); }
	void Skin() override { ApplyTheme(); }
	bool CheckControls(String& error) {
		UiThemeMode original = UiTheme::GetMode();
		bool valid = true;
		for(int transition = 0; transition < 4; ++transition) {
			UiTheme::Set(UiTheme::GetMode() == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark); Ctrl::SwapDarkLight(); ApplyTheme();
			auto surface = UiTheme::ResolvePanel(); auto edit = UiTheme::ResolveEdit();
			for(UiPanel* panel : { &catalogue_panel_, &usage_panel_ }) if(panel->GetStyle().palette.face[ST_NORMAL].color != surface.palette.face[ST_NORMAL].color) valid = false;
			if(document_.GetStyle().page_face != surface.palette.face[ST_NORMAL].color || document_.GetStyle().palette.ink[ST_NORMAL] != edit.palette.ink[ST_NORMAL]) valid = false;
			for(FormRow* row : { &remote_, &source_, &metadata_, &picker_, &project_, &code_, &output_ }) if(row->edit.GetStyle().palette.face[ST_NORMAL].color != edit.palette.face[ST_NORMAL].color) valid = false;
		}
		if(UiTheme::GetMode() != original) valid = false;
		if(!valid) { error = "Utility panels, document or input controls retained a previous theme."; return false; }
		prepared_ = "old staged path"; source_.edit.SetTextUtf8("a newly selected source"); source_.edit.WhenChange();
		if(!prepared_.IsEmpty() || install_.IsEnabled()) { error = "Editing the source did not invalidate the prepared refresh."; return false; }
		mode_.Select(1);
		if(pages_.GetActivePage() != 1) { error = "The usage tool selector did not switch pages."; return false; }
		return true;
	}
	bool Key(dword key, int count) override { if(key == K_ESCAPE) { RequestClose(); return true; } return TopWindow::Key(key, count); }
private:
	enum Action { Download, Compare, Prepare, Install, Scan, Apply };
	enum { TIMEID_POLL = TopWindow::TIMEID_COUNT, TIMEID_COUNT };
	void ApplyTheme() {
		title_.SetCustomStyle(UiTheme::ResolveTitleCard());
		catalogue_panel_.ClearCustomStyle().SetRadius(DPI(8)); usage_panel_.ClearCustomStyle().SetRadius(DPI(8));
		auto style = UiTheme::ResolveDoc(); auto surface = UiTheme::ResolvePanel(); auto edit = UiTheme::ResolveEdit();
		style.palette = surface.palette; for(int s = 0; s < 4; ++s) style.palette.ink[s] = edit.palette.ink[s];
		style.skin = StyledSkin(); style.metrics.radius = DPI(8); style.page_face = surface.palette.face[ST_NORMAL].color; style.page_frame = surface.palette.frame[ST_NORMAL];
		style.font = SansSerifZ(11); style.paragraph_gap = DPI(8); style.page_padding = DPI(16); style.caret_ink = edit.caret_color; style.selection_fill = edit.selection_color;
		document_.SetCustomStyle(style);
	}
	void SetDocument(const VectorMap<String, String>& sections) {
		WString text; Vector<UiDocBlock> blocks;
		for(int i = 0; i < sections.GetCount(); ++i) {
			UiDocBlock& b = blocks.Add(); b.range.from = text.GetCount(); text << sections[i].ToWString(); b.range.to = text.GetCount(); b.role = sections.GetKey(i); text.Cat('\n');
		}
		document_.Model().Clear(); document_.Model().Replace(UiDocRange(0, 0), text);
		for(const auto& b : blocks) document_.Model().AddBlock(b.range, b.role);
		document_.SetSelection(UiDocRange(0, 0));
	}
	void ShowHelp() {
		VectorMap<String, String> text;
		if((int)mode_.GetData() == 0) {
			text.Add("heading.2", "Keep the symbol catalogue current");
			text.Add("paragraph", "Use this when you have a newer Material Symbols download, or want to check for missing icons. This compares the regular unfilled 24 px SVGs in all three styles. Filled and other optical sizes are separate variants.");
			text.Add("list.bullet", "Material download: select the repository root containing symbols/web. Download latest obtains a fresh snapshot from the address above and category metadata from Google Fonts. It needs Git on PATH and an internet connection.");
			text.Add("list.bullet", "Compare: checks names, style families and SVG artwork against the catalogue built into this executable. It reports additions, changes, retired entries and unreadable files. Category memberships can make the gallery count larger than the number of unique names.");
			text.Add("list.bullet", "Prepare refresh: writes generated C++ headers to a separate folder. Existing category/name/style IDs stay stable. Removed upstream artwork is retained so saved collections still resolve. Category metadata is required; incomplete or unreadable snapshots cannot be prepared.");
			text.Add("list.bullet", "Install prepared refresh: backs up existing generated files, then copies the prepared catalogue into your Picker source folder. Rebuild with Build.ps1 and reopen the latest bin/windows-x64 executable to use it. Collection projects are left in place.");
			text.Add("list.bullet", "Reports and backups: each operation has its own folder with full JSON evidence, source commit and hashes. Compare reads SVGs only when requested; normal Picker startup still indexes names and loads artwork as needed. An existing folder is not proof of being the latest download.");
		}
		else {
			text.Add("heading.2", "See where an application's icons are used");
			text.Add("paragraph", "Save the collection as a .symbolpicker project, choose the source folder of the application that uses it, and choose Scan usage. Review the matched aliases and file paths before applying warnings.");
			text.Add("list.bullet", "The scan looks for whole identifiers in C++ headers/source and JSON, including quoted names, comments and inactive code. It skips generated folders, builds, binaries and the project's own icon definitions. A match is evidence to review, not proof that code runs.");
			text.Add("list.bullet", "Apply usage warnings marks matched collection items and adds the source paths to their usage note. It preserves manual notes and existing marks, including marks with no new match. It creates a backup and refuses to overwrite a project changed since scanning.");
			text.Add("list.bullet", "Save any open edits before applying; then reload the project in Symbol Picker to see its blue usage dots. Deletion still gives a warning rather than locking the icon. Dynamically constructed names and callers outside this folder may be missed, so you can also mark usage manually.");
		}
		SetDocument(text);
	}
	void ShowResult() {
		VectorMap<String, String> text;
		text.Add("heading.2", result_.ok ? "Ready to review" : "Operation needs attention");
		if(!result_.summary.IsEmpty()) text.Add("paragraph", result_.summary);
		if(!result_.error.IsEmpty()) text.Add("paragraph", result_.error);
		for(const char* key : {"commit", "report_path", "stage", "backup", "snapshot", "metadata"}) {
			int index = result_.report.Find(key); if(index >= 0) text.Add("list.bullet", String(key) + ": " + AsString(result_.report.GetValue(index)));
		}
		for(const char* key : {"missing_variants", "changed_artwork", "removed_upstream_variants_retained", "read_errors", "incomplete_upstream_styles", "live_metadata_supported_symbols_missing_in_snapshot", "items"}) {
			int index = result_.report.Find(key); if(index < 0 || !IsValueArray(result_.report.GetValue(index))) continue;
			ValueArray rows(result_.report.GetValue(index)); if(rows.IsEmpty()) continue;
			String heading = key; heading.Replace("_", " "); text.Add("heading.3", heading);
			int shown = 0;
			for(Value row : rows) {
				if(!IsValueMap(row)) { text.Add("list.bullet", AsString(row)); }
				else {
					ValueMap m(row);
					if(String(key) == "items") {
						Value matches = m["matches"]; if(!IsValueArray(matches) || ValueArray(matches).IsEmpty()) continue;
						ValueArray files(matches); String lines = AsString(m["alias"]); for(Value file : files) lines << "\n" << AsString(file); text.Add("list.bullet", lines);
					}
					else text.Add("list.bullet", AsJSON(m));
				}
				if(++shown >= 20) { text.Add("paragraph", "The full report contains the remaining entries. Use Copy report to inspect all evidence."); break; }
			}
		}
		if(!prepared_.IsEmpty() && action_ == Prepare) text.Add("paragraph", "Review this report, then choose Install prepared refresh when you are ready to update the source folder.");
		SetDocument(text);
	}
	void UpdateActions() {
		mode_.Enable(!busy_); help_.Enable(!busy_); pages_.Enable(!busy_); output_.box.Enable(!busy_);
		install_.Enable(!busy_ && !prepared_.IsEmpty()); apply_.Enable(!busy_ && scan_result_.ok && !scan_result_.project_after.IsEmpty());
		cancel_.Enable(busy_); close_.Enable(true); copy_.Enable(!busy_ && result_.report.GetCount() > 0);
	}
	void RequestClose() { if(busy_) { closing_ = true; job_.cancelled = true; status_.SetText("Cancelling before closing…"); } else Close(); }
	void Start(Action action) {
		if(busy_) return;
		String output = output_.Text(); if(output.IsEmpty()) { Exclamation("Choose a folder for reports and backups first."); return; }
		if(action == Apply && !PromptYesNo("Have you saved any open collection edits? This applies the reviewed usage warnings to the saved project. Reload the project afterwards.")) return;
		if(action == Install && !PromptYesNo("Install the prepared catalogue in the selected Picker source folder? Current generated files will be backed up. Rebuild Symbol Picker afterwards.")) return;
		String remote = remote_.Text(), source = source_.Text(), metadata = metadata_.Text(), picker = picker_.Text(), project = project_.Text(), code = code_.Text(), prepared = prepared_;
		Vector<SymbolPickerSourceRow> rows;
		if(action == Compare || action == Prepare) GetSymbolPickerSourceRows(rows);
		scan_input_ = scan_result_; result_ = SymbolPickerMaintenanceResult(); action_ = action; done_ = false; job_.cancelled = false; busy_ = true;
		if(action == Compare || action == Prepare || action == Download) prepared_.Clear();
		if(action == Scan) scan_result_ = SymbolPickerMaintenanceResult();
		job_.Status("Starting…"); status_.SetText("Working… You can cancel."); UpdateActions();
		rows_ = pick(rows);
		if(!worker_.Run([=] {
			try {
				switch(action) {
				case Download: DownloadSymbolPickerSources(remote, output, job_, result_); break;
				case Compare: case Prepare: CompareSymbolPickerSources(rows_, source, metadata, output, action == Prepare, job_, result_); break;
				case Install: InstallSymbolPickerRefresh(prepared, picker, job_, result_); break;
				case Scan: ScanSymbolPickerUsage(project, code, output, job_, result_); break;
				case Apply: ApplySymbolPickerUsage(scan_input_, output, job_, result_); break;
				}
			}
			catch(const Exc& error) { result_.error = error; }
			catch(...) { result_.error = "The operation failed unexpectedly. Check the selected files and try again."; }
			done_.store(true);
		})) { busy_ = false; result_.error = "Could not start the background job."; ShowResult(); UpdateActions(); return; }
		SetTimeCallback(-100, [=] { Poll(); }, TIMEID_POLL);
	}
	void Poll() {
		if(!done_.load()) { String progress = job_.GetStatus(); status_.SetText(progress.Left(90)); status_.Tip(progress); return; }
		KillTimeCallback(TIMEID_POLL); worker_.Wait(); busy_ = false; rows_.Clear();
		if(result_.ok && action_ == Download) { source_.edit.SetData(result_.snapshot); metadata_.edit.SetData(result_.metadata); }
		if(result_.ok && action_ == Prepare) prepared_ = result_.stage;
		if(result_.ok && action_ == Scan) scan_result_ = result_;
		if(result_.ok && action_ == Apply) scan_result_ = SymbolPickerMaintenanceResult();
		status_.SetText(result_.ok ? "Complete. Review the results below." : "Stopped. See the details below.");
		ShowResult(); UpdateActions(); if(closing_) Close();
	}
	UiBoxLayout root_ { UiDirection::V }, mode_row_ { UiDirection::H }, catalogue_form_ { UiDirection::V }, usage_form_ { UiDirection::V }, catalogue_actions_ { UiDirection::H }, usage_actions_ { UiDirection::H }, footer_ { UiDirection::H };
	UiTitleCard title_; UiLabel mode_label_, catalogue_hint_, usage_hint_, usage_note_, status_;
	UiDropdown mode_; UiPanel catalogue_panel_, usage_panel_; UiStack pages_;
	FormRow remote_, source_, metadata_, picker_, project_, code_, output_;
	UiButton help_, download_, compare_, prepare_, install_, scan_, apply_, cancel_, close_, copy_;
	UtilityDocument document_;
	Thread worker_; SymbolPickerMaintenanceJob job_; std::atomic<bool> done_ { false };
	bool busy_ = false, closing_ = false; Action action_ = Compare;
	Vector<SymbolPickerSourceRow> rows_;
	SymbolPickerMaintenanceResult result_, scan_result_, scan_input_;
	String prepared_;
};
}

void ShowSymbolPickerUtilities(One<TopWindow>& window, const String& project_path) {
	if(window && window->IsOpen()) { window->SetForeground(); return; }
	window = new UtilitiesWindow(project_path); window->OpenMain();
}
bool RunSymbolPickerUtilitiesTests(String& error) {
	UtilitiesWindow window(String{});
	return window.CheckControls(error);
}
}
