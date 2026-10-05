#include "SymbolPickerMaintenance.h"
#include <plugin/z/z.h>
#ifdef PLATFORM_WIN32
#include <winhttp.h>
#endif

namespace Upp {

void SymbolPickerMaintenanceJob::Check() const { if(cancelled.load()) throw Exc("Cancelled. Your current catalogue and project were not changed."); }
void SymbolPickerMaintenanceJob::Status(const String& text) { Mutex::Lock lock(mutex); progress = text; }
String SymbolPickerMaintenanceJob::GetStatus() { Mutex::Lock lock(mutex); return progress; }

namespace {
const char* folders[] = { "materialsymbolsoutlined", "materialsymbolsrounded", "materialsymbolssharp" };
const char* styles[] = { "outlined", "rounded", "sharp" };
const char* enums[] = { "OUTLINED", "ROUNDED", "SHARP" };
String Key(const String& name, int style) { return name + "/" + styles[style]; }
String Identifier(const String& value) {
	String text;
	for(int c : value) text.Cat(IsAlNum(c) || c == '_' ? ToLower(c) : '_');
	if(text.IsEmpty()) text = "uncategorized";
	if(IsDigit(text[0])) text = "icon_" + text;
	return text;
}
String Category(const String& raw) {
	String text = ToLower(TrimBoth(raw));
	if(text == "audio&video" || text == "audio & video" || text == "audio / video" || text == "av" || text == "audiovideo") return "audio_video";
	if(text == "ui action" || text == "ui actions" || text == "ui_action" || text == "ui_actions" || text == "uiaction") return "ui_action";
	if(text == "images") return "image";
	text = Identifier(text);
	while(text.Find("__") >= 0) text.Replace("__", "_");
	while(text.EndsWith("_")) text.Trim(text.GetCount() - 1);
	if(text.GetCount() > 3 && text.EndsWith("s")) text.Trim(text.GetCount() - 1);
	return text;
}
Value Get(const ValueMap& map, const char* key, Value def = Null) {
	int i = map.Find(key); return i < 0 ? def : map.GetValue(i);
}
ValueMap ReadJson(const String& path) {
	String data = LoadFile(path);
	if(data.IsVoid()) throw Exc("Cannot read " + path);
	if(data.StartsWith("\xEF\xBB\xBF")) data = data.Mid(3);
	Value value = ParseJSON(data);
	if(!IsValueMap(value)) throw Exc("Expected a JSON object in " + path);
	return ValueMap(value);
}
void Write(const String& path, const String& data) {
	RealizeDirectory(GetFileFolder(path));
	if(!SaveFile(path, data)) throw Exc("Cannot write " + path);
}
void WriteJson(const String& path, const ValueMap& value) { Write(path, AsJSON(value, true) + "\n"); }
String WorkFolder(const String& output, const char* prefix) {
	String folder = AppendFileName(GetFullPath(output), String(prefix) + "-" + FormatInt64((int64)Random64()));
	if(!RealizeDirectory(folder)) throw Exc("Cannot create working folder " + folder);
	return folder;
}
String Git(const String& source, const Vector<String>& args, SymbolPickerMaintenanceJob& job, const String& input = String()) {
	Vector<String> argv;
	argv << "-c" << "core.hooksPath=" << "-c" << "safe.directory=" + GetFullPath(source);
	if(!source.IsEmpty()) argv << "-C" << source;
	for(const auto& arg : args) argv.Add(arg);
#ifdef PLATFORM_WIN32
	struct ProcessTree { HANDLE handle; ~ProcessTree() { if(handle) CloseHandle(handle); } } tree { CreateJobObjectW(NULL, NULL) };
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
	limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if(!tree.handle || !SetInformationJobObject(tree.handle, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) throw Exc("Cannot create a cancellable download process.");
#endif
	LocalProcess process;
	if(!process.Start("git", argv)) throw Exc("Git could not start. Install Git and make it available on PATH to download a snapshot.");
#ifdef PLATFORM_WIN32
	if(!AssignProcessToJobObject(tree.handle, process.GetProcessHandle())) { process.Kill(); throw Exc("Cannot track Git's download process. Try downloading the source separately and select its folder."); }
#endif
	for(int pos = 0; pos < input.GetCount(); pos += 16384) { if(job.cancelled.load()) { process.Kill(); job.Check(); } process.Write(input.Mid(pos, 16384)); }
	process.CloseWrite();
	String output;
	while(process.IsRunning()) {
		if(job.cancelled.load()) { process.Kill(); job.Check(); }
		String chunk; process.Read(chunk); output.Cat(chunk);
		if(output.GetCount() > 8 * 1024 * 1024) { process.Kill(); throw Exc("Git returned an unexpectedly large response."); }
		Thread::Sleep(20);
	}
	String tail; process.Read(tail); output.Cat(tail);
	if(process.GetExitCode() != 0) throw Exc("Git failed: " + output.Right(3000));
	return TrimBoth(output);
}
String GitInfo(const String& source, std::initializer_list<const char*> args, SymbolPickerMaintenanceJob& job) {
	Vector<String> argv; for(auto arg : args) argv.Add(arg);
	try { return Git(source, argv, job); }
	catch(const Exc&) { job.Check(); return "unversioned local folder"; }
}
String CanonicalNode(const XmlNode& node) {
	if(node.IsText()) { String text = TrimBoth(node.GetText()); return text.IsEmpty() ? String() : AsJSON(text); }
	String value = node.IsTag() ? "<" + node.GetTag() : "";
	VectorMap<String, String> attrs;
	for(int i = 0; i < node.GetAttrCount(); ++i) attrs.Add(node.AttrId(i), node.Attr(i));
	SortByKey(attrs);
	for(int i = 0; i < attrs.GetCount(); ++i) value << AsJSON(attrs.GetKey(i)) << AsJSON(attrs[i]);
	if(node.IsTag()) value << ">";
	for(const auto& child : node) value << CanonicalNode(child);
	if(node.IsTag()) value << "</" << node.GetTag() << ">";
	return value;
}
String CanonicalSvg(const String& svg) {
	XmlNode root = ParseXML(svg);
	if(root.FindTag("svg") < 0) throw Exc("The file has no SVG root element.");
	return CanonicalNode(root);
}
struct Source : Moveable<Source> { String name, path, svg, canonical; int style = 0; };
ValueMap Variant(const Source& row) { ValueMap m; m.Add("name", row.name); m.Add("style", styles[row.style]); m.Add("path", row.path); return m; }

// WinHTTP uses the Windows certificate store; no Python or bundled SSL runtime.
String DownloadMetadata(SymbolPickerMaintenanceJob& job) {
#ifdef PLATFORM_WIN32
	struct Handle { HINTERNET h; ~Handle() { if(h) WinHttpCloseHandle(h); } };
	Handle session { WinHttpOpen(L"SymbolPicker/1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0) };
	if(!session.h) throw Exc("Cannot open a secure download session.");
	WinHttpSetTimeouts(session.h, 10000, 10000, 10000, 10000);
	Handle connection { WinHttpConnect(session.h, L"fonts.google.com", INTERNET_DEFAULT_HTTPS_PORT, 0) };
	Handle request { connection.h ? WinHttpOpenRequest(connection.h, L"GET", L"/metadata/icons?incomplete=1&key=material_symbols", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : NULL };
	if(!request.h || !WinHttpSendRequest(request.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request.h, NULL)) throw Exc("Cannot download Google Fonts category metadata. Check your network, or choose an existing metadata JSON file.");
	DWORD code = 0, length = sizeof(code);
	WinHttpQueryHeaders(request.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &length, WINHTTP_NO_HEADER_INDEX);
	if(code != 200) throw Exc(Format("Metadata server returned HTTP %d.", (int)code));
	String data;
	for(;;) {
		job.Check();
		char buffer[16384]; DWORD read = 0;
		if(!WinHttpReadData(request.h, buffer, sizeof(buffer), &read)) throw Exc("Metadata download was interrupted.");
		if(!read) break;
		data.Cat(buffer, read);
		if(data.GetCount() > 32 * 1024 * 1024) throw Exc("Metadata response is too large.");
	}
	int start = data.Find('{');
	if(start < 0) throw Exc("The metadata response is not JSON.");
	data = data.Mid(start);
	Value value = ParseJSON(data);
	if(!IsValueMap(value) || !IsValueArray(Get(ValueMap(value), "icons"))) throw Exc("The metadata response has no icon list.");
	return data;
#else
	throw Exc("Automatic metadata downloads currently use Windows HTTPS. Choose an existing metadata JSON file on this platform.");
#endif
}

bool SkipFolder(const String& name) {
	String lower = ToLower(name);
	return lower == ".git" || lower == "build" || lower == "bin" || lower == "out" || lower == "generated" || lower == "node_modules" || lower == ".svn";
}
void CodeFiles(const String& folder, Vector<String>& files, SymbolPickerMaintenanceJob& job) {
	job.Check();
	FindFile find(AppendFileName(folder, "*"));
	while(find) {
		String name = find.GetName();
		if(name != "." && name != "..") {
			if(find.IsFolder()) {
				// Do not follow directory symlinks/junctions outside the selected tree.
#ifdef PLATFORM_WIN32
				DWORD attrs = GetFileAttributesW(ToSystemCharsetW(find.GetPath()));
				bool link = attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT);
#else
				bool link = find.IsSymLink();
#endif
				if(!SkipFolder(name) && !link) CodeFiles(find.GetPath(), files, job);
			}
			else {
				String ext = ToLower(GetFileExt(name));
				if(ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".cc" || ext == ".cxx" || ext == ".json") files.Add(find.GetPath());
			}
		}
		find.Next();
	}
}
String ScanText(String text) {
	if(text.GetCount() >= 2 && ((byte)text[0] == 255 && (byte)text[1] == 254 || (byte)text[0] == 254 && (byte)text[1] == 255)) {
		bool little = (byte)text[0] == 255; WString wide;
		for(int i = 2; i + 1 < text.GetCount(); i += 2) wide.Cat(little ? (byte)text[i] | (byte)text[i+1] << 8 : (byte)text[i] << 8 | (byte)text[i+1]);
		return wide.ToString();
	}
	return text;
}
}

void DownloadSymbolPickerSources(const String& remote, const String& output,
                                SymbolPickerMaintenanceJob& job, SymbolPickerMaintenanceResult& result)
{
	try {
		if(!remote.StartsWith("https://") || remote.Find('@') >= 0 || remote.Find('?') >= 0 || remote.Find('#') >= 0 || remote.Find('\r') >= 0 || remote.Find('\n') >= 0 || remote.Find('\t') >= 0 || remote.Find(' ') >= 0) throw Exc("Enter an HTTPS Git repository address, for example https://github.com/Trilec/material-design-icons.git.");
		job.Check(); job.Status("Downloading a separate snapshot. Your existing download is left in place.");
		String folder = WorkFolder(output, "download");
		result.snapshot = AppendFileName(folder, "material-design-icons");
		Vector<String> args; args << "clone" << "--depth=1" << "--filter=blob:none" << "--no-checkout" << "--" << remote << result.snapshot;
		Git("", args, job);
		String patterns = "/LICENSE\n/README.md\n";
		for(int style = 0; style < 3; ++style) patterns << "/symbols/web/*/" << folders[style] << "/*_24px.svg\n";
		// A few patterns avoid quadratic work against the repository's millions of
		// axis variants. Comparison still selects the exact <name>_24px.svg file.
		patterns << "!**/*_fill1_24px.svg\n!**/*_wght*_24px.svg\n!**/*_grad*_24px.svg\n";
		job.Status("Checking out only the baseline SVGs for the three styles.");
		args.Clear(); args << "sparse-checkout" << "set" << "--no-cone" << "--stdin"; Git(result.snapshot, args, job, patterns);
		args.Clear(); args << "checkout"; Git(result.snapshot, args, job);
		result.report.Add("remote", remote); result.report.Add("snapshot", result.snapshot);
		result.report.Add("commit", GitInfo(result.snapshot, {"rev-parse", "HEAD"}, job));
		job.Status("Downloading category metadata from Google Fonts.");
		result.metadata = AppendFileName(folder, "material-symbols-metadata.json");
		Write(result.metadata, DownloadMetadata(job));
		result.report.Add("metadata", result.metadata);
		WriteJson(AppendFileName(folder, "download.json"), result.report);
		result.summary = "Download complete. This is a fresh, pinned snapshot. Choose Compare to see what differs from the catalogue in this version of Symbol Picker.";
		result.ok = true;
	}
	catch(const Exc& error) { result.error = error; }
}

void CompareSymbolPickerSources(const Vector<SymbolPickerSourceRow>& current,
                               const String& source, const String& metadata,
                               const String& output, bool prepare,
                               SymbolPickerMaintenanceJob& job, SymbolPickerMaintenanceResult& result)
{
	try {
		job.Check(); job.Status("Finding the regular 24 px Outlined, Rounded and Sharp SVGs.");
		String web = AppendFileName(source, "symbols/web");
		if(!DirectoryExists(web)) throw Exc("Choose the material-design-icons folder containing symbols/web. The older svg folder contains a different icon family.");
		if(current.IsEmpty()) throw Exc("The current built-in catalogue is empty.");
		VectorMap<String, Source> available;
		Index<String> names, prior, prior_names;
		ValueArray errors, incomplete, missing, removed, changed;
		FindFile find(AppendFileName(web, "*"));
		while(find) {
			job.Check();
			String name = find.GetName();
			if(find.IsFolder() && name != "." && name != "..") {
				bool found = false;
				for(int style = 0; style < 3; ++style) {
					String path = "symbols/web/" + name + "/" + folders[style] + "/" + name + "_24px.svg";
					String full = AppendFileName(source, path);
					if(!FileExists(full)) continue;
					found = true;
					Source& row = available.Add(Key(name, style)); row.name = name; row.path = path; row.style = style;
					row.svg = LoadFile(full);
					try { if(row.svg.IsVoid()) throw Exc("Cannot read file."); row.canonical = CanonicalSvg(row.svg); }
					catch(const Exc& error) { ValueMap m; m.Add("path", path); m.Add("error", String(error)); errors.Add(m); }
				}
				if(found) names.FindAdd(name);
			}
			find.Next();
		}
		if(available.IsEmpty()) throw Exc("No regular Material Symbols SVGs were found in that folder.");
		SortByKey(available); SortIndex(names);
		for(const auto& name : names) for(int style = 0; style < 3; ++style) if(available.Find(Key(name, style)) < 0) { ValueMap m; m.Add("name", name); m.Add("missing_style", styles[style]); incomplete.Add(m); }
		VectorMap<String, String> old_canonical;
		for(const auto& row : current) {
			job.Check();
			String key = Key(row.name, row.style); prior.FindAdd(key); prior_names.FindAdd(row.name);
			if(old_canonical.Find(key) < 0) old_canonical.Add(key, CanonicalSvg(ZDecompress(Base64Decode(row.packed_svg))));
		}
		Index<String> missing_names;
		for(int i = 0; i < available.GetCount(); ++i) {
			int old = old_canonical.Find(available.GetKey(i));
			if(old < 0) { missing.Add(Variant(available[i])); missing_names.FindAdd(available[i].name); }
			else if(available[i].canonical != old_canonical[old]) changed.Add(Variant(available[i]));
		}
		for(const auto& key : prior) if(available.Find(key) < 0) removed.Add(key);
		ValueMap categories;
		ValueArray supported_missing, unsupported_snapshot;
		int supported_count = 0;
		String metadata_hash;
		if(!metadata.IsEmpty()) {
			job.Status("Checking category metadata and supported symbol families.");
			ValueMap meta = ReadJson(metadata);
			Value icons = Get(meta, "icons");
			if(!IsValueArray(icons)) throw Exc("The metadata JSON has no icons array.");
			Index<String> supported;
			for(Value value : ValueArray(icons)) {
				if(!IsValueMap(value)) throw Exc("The metadata contains an invalid icon entry.");
				ValueMap icon(value); String name = AsString(Get(icon, "name"));
				categories.Set(name, Get(icon, "categories", ValueArray()));
				Index<String> excluded;
				Value families = Get(icon, "unsupported_families", ValueArray());
				if(IsValueArray(families)) for(Value family : ValueArray(families)) excluded.FindAdd(AsString(family));
				if(excluded.Find("Material Symbols Outlined") < 0 || excluded.Find("Material Symbols Rounded") < 0 || excluded.Find("Material Symbols Sharp") < 0) supported.FindAdd(name);
			}
			for(const auto& name : supported) if(names.Find(name) < 0) supported_missing.Add(name);
			for(const auto& name : names) if(supported.Find(name) < 0) unsupported_snapshot.Add(name);
			supported_count = supported.GetCount(); metadata_hash = SHA256String(LoadFile(metadata));
		}
		result.report.Add("source", GetFullPath(source));
		result.report.Add("remote", GitInfo(source, {"remote", "get-url", "origin"}, job));
		String commit = GitInfo(source, {"rev-parse", "HEAD"}, job);
		result.report.Add("commit", commit);
		result.report.Add("comparison", "Catalogue compiled into this version of Symbol Picker; existing category/name/style IDs are preserved.");
		result.report.Add("selector", "symbols/web/<name>/<outlined|rounded|sharp>/<name>_24px.svg; regular, fill=0. Filled/optical-size variants are separate.");
		result.report.Add("current_category_rows", current.GetCount()); result.report.Add("current_unique_names", prior_names.GetCount()); result.report.Add("upstream_unique_names", names.GetCount());
		ValueArray added_names; for(const auto& name : missing_names) added_names.Add(name);
		result.report.Add("upstream_name_style_pairs", available.GetCount()); result.report.Add("missing_variants", missing); result.report.Add("missing_names", added_names);
		result.report.Add("changed_artwork", changed); result.report.Add("removed_upstream_variants_retained", removed); result.report.Add("read_errors", errors); result.report.Add("incomplete_upstream_styles", incomplete);
		result.report.Add("metadata_sha256", metadata_hash); result.report.Add("live_metadata_supported_symbol_names", supported_count); result.report.Add("live_metadata_supported_symbols_missing_in_snapshot", supported_missing); result.report.Add("snapshot_names_not_supported_by_live_metadata", unsupported_snapshot);
		bool blocked = !errors.IsEmpty() || !incomplete.IsEmpty();
		if(prepare && metadata.IsEmpty()) throw Exc("Choose category metadata, or use Download latest to obtain it before preparing a refresh.");
		String folder = WorkFolder(output, prepare ? "prepare" : "compare");
		if(prepare && !blocked) {
			job.Status("Preparing headers in a separate folder. Keeping existing IDs and retired artwork.");
			Vector<SymbolPickerSourceRow> rows = clone(current);
			for(auto& row : rows) { int i = available.Find(Key(row.name, row.style)); if(i >= 0) { row.packed_svg = Base64Encode(ZCompress(available[i].svg)); row.source = available[i].path; } }
			Index<String> ids;
			for(const auto& row : rows) ids.FindAdd(row.category + "/" + Key(row.name, row.style));
			for(int i = 0; i < available.GetCount(); ++i) if(prior.Find(available.GetKey(i)) < 0) {
				const Source& row = available[i]; Value cats = Get(categories, ~row.name, ValueArray());
				ValueArray cat_array; if(IsValueArray(cats)) cat_array = ValueArray(cats);
				if(cat_array.IsEmpty()) cat_array.Add("Uncategorized");
				for(Value cat : cat_array) {
					String category = Category(AsString(cat)); String id = category + "/" + available.GetKey(i);
					if(ids.Find(id) >= 0) continue;
					ids.Add(id); auto& added = rows.Add(); added.category = category; added.name = row.name; added.style = row.style; added.source = row.path; added.packed_svg = Base64Encode(ZCompress(row.svg));
				}
			}
			Sort(rows, [](const SymbolPickerSourceRow& a, const SymbolPickerSourceRow& b) { return a.category == b.category ? a.style == b.style ? a.name < b.name : a.style < b.style : a.category < b.category; });
			result.stage = AppendFileName(folder, "Generated");
			ValueMap files, hashes; Vector<String> cats;
			for(int start = 0; start < rows.GetCount();) {
				job.Check(); String cat = rows[start].category;
				if(Identifier(cat) != cat) throw Exc("Invalid category identifier: " + cat);
				cats.Add(cat); String guard = "UPP_MATERIAL_SYMBOLS_ICONS_" + ToUpper(cat) + "_H";
				String header = "// Generated by Symbol Picker Utilities; source commit " + commit + "\n// FORMAT: KICONS_V1_NO_DECODERS\n#ifndef " + guard + "\n#define " + guard + "\n#include <Core/Core.h>\n#include <plugin/z/z.h>\nnamespace " + cat + " {\nusing namespace Upp;\nenum Style : uint8 { OUTLINED = 0, ROUNDED = 1, SHARP = 2 };\nstruct BaseSVGIcon { const char* category; Style style; const char* name; const char* source; const char* b64zIcon; };\n";
				String table = "static const BaseSVGIcon kIcons[] = {\n";
				while(start < rows.GetCount() && rows[start].category == cat) {
					const auto& row = rows[start++]; String symbol = "svg_" + AsString(start);
					header << "static const char " << symbol << "[] =\n";
					for(int pos = 0; pos < row.packed_svg.GetCount(); pos += 80) header << "    \"" << row.packed_svg.Mid(pos, 80) << "\"\n";
					header << ";\n";
					table << "    { " << AsJSON(row.category) << ", " << enums[row.style] << ", " << AsJSON(row.name) << ", " << AsJSON(row.source) << ", " << symbol << " },\n";
				}
				header << table << "};\nstatic const int kIconCount = sizeof(kIcons) / sizeof(kIcons[0]);\n}\n#endif\n";
				String filename = "icons_" + cat + ".h"; Write(AppendFileName(result.stage, filename), header); files.Add(filename, SHA256String(header));
			}
			String manifest = "// Generated catalogue includes and table iteration.\n";
			for(const auto& cat : cats) manifest << "#include \"icons_" << cat << ".h\"\n";
			manifest << "\n#define SYMBOLPICKER_GENERATED_ICON_TABLES(OP) \\\n";
			for(int i = 0; i < cats.GetCount(); ++i) manifest << "\tOP(" << cats[i] << ")" << (i + 1 < cats.GetCount() ? " \\\n" : "\n");
			Write(AppendFileName(result.stage, "CatalogTables.h"), manifest); files.Add("CatalogTables.h", SHA256String(manifest));
			String license = LoadFile(AppendFileName(source, "LICENSE"));
			if(license.IsVoid()) throw Exc("Source LICENSE is missing; cannot prepare a redistributable catalogue.");
			Write(AppendFileName(result.stage, "MaterialSymbols.LICENSE"), license); files.Add("MaterialSymbols.LICENSE", SHA256String(license));
			for(const auto& row : available) hashes.Add(row.path, SHA256String(row.svg));
			ValueMap provenance; provenance.Add("remote", Get(result.report, "remote")); provenance.Add("commit", commit); provenance.Add("selector", Get(result.report, "selector")); provenance.Add("metadata_sha256", metadata_hash); provenance.Add("source_svg_sha256", hashes);
			String provenance_text = AsJSON(provenance, true) + "\n";
			Write(AppendFileName(result.stage, "provenance.json"), provenance_text); files.Add("provenance.json", SHA256String(provenance_text));
			ValueMap receipt; receipt.Add("files", files); receipt.Add("category_rows", rows.GetCount()); receipt.Add("commit", commit);
			WriteJson(AppendFileName(result.stage, "refresh.json"), receipt);
			result.report.Add("staged_category_rows", rows.GetCount()); result.report.Add("staged_headers", cats.GetCount()); result.report.Add("stage", result.stage);
		}
		job.Check();
		String report_path = AppendFileName(folder, "comparison.json"); result.report.Add("report_path", report_path); WriteJson(report_path, result.report);
		result.summary = Format("%d current names; %d downloaded names. %d added names / %d variants; %d changed variants; %d retired variants retained. %d unreadable files; %d incomplete styles.", prior_names.GetCount(), names.GetCount(), missing_names.GetCount(), missing.GetCount(), changed.GetCount(), removed.GetCount(), errors.GetCount(), incomplete.GetCount());
		if(blocked) throw Exc("The comparison report was saved, but this snapshot has unreadable files or incomplete style families. A refresh cannot be installed.");
		result.ok = true;
	}
	catch(const Exc& error) { result.error = error; result.stage.Clear(); }
}

void InstallSymbolPickerRefresh(const String& stage, const String& picker_source,
                               SymbolPickerMaintenanceJob& job, SymbolPickerMaintenanceResult& result)
{
	String target = AppendFileName(GetFullPath(picker_source), "UiSymbolPicker/Generated");
	Vector<String> installed, created;
	String backup;
	String package = AppendFileName(GetFullPath(picker_source), "UiSymbolPicker/UiSymbolPicker.upp");
	bool package_written = false;
	try {
		job.Check();
		if(!FileExists(AppendFileName(picker_source, "UiSymbolPicker/UiSymbolPicker.upp")) || !DirectoryExists(target)) throw Exc("Choose the Symbol Picker source folder containing UiSymbolPicker/UiSymbolPicker.upp.");
		ValueMap receipt = ReadJson(AppendFileName(stage, "refresh.json")); Value value = Get(receipt, "files");
		if(!IsValueMap(value)) throw Exc("That folder is not a prepared Symbol Picker refresh.");
		ValueMap files(value);
		if(files.GetCount() < 4 || files.Find("CatalogTables.h") < 0 || files.Find("provenance.json") < 0 || files.Find("MaterialSymbols.LICENSE") < 0) throw Exc("The refresh receipt is incomplete.");
		for(int i = 0; i < files.GetCount(); ++i) {
			job.Check(); String name = AsString(files.GetKey(i));
			if(GetFileName(name) != name || name.Find('/') >= 0 || name.Find('\\') >= 0 || name.Find(':') >= 0 || !(name == "CatalogTables.h" || name == "provenance.json" || name == "MaterialSymbols.LICENSE" || name.StartsWith("icons_") && name.EndsWith(".h"))) throw Exc("Invalid refresh filename.");
			String content = LoadFile(AppendFileName(stage, name));
			if(content.IsVoid() || SHA256String(content) != AsString(files.GetValue(i))) throw Exc("A prepared file has changed. Prepare the refresh again before installing it.");
		}
		String package_before = LoadFile(package), package_after = package_before;
		int insert = package_after.Find("\tGenerated/CatalogTables.h,");
		if(insert < 0) insert = package_after.Find("\tmain.cpp;");
		if(insert < 0) throw Exc("The Picker package's file list could not be recognized. Its current files were left unchanged.");
		for(int i = 0; i < files.GetCount(); ++i) {
			String name = AsString(files.GetKey(i));
			if(name.StartsWith("icons_") && name.EndsWith(".h") && package_before.Find("Generated/" + name) < 0) {
				String entry = "\tGenerated/" + name + ",\n"; package_after.Insert(insert, entry); insert += entry.GetCount();
			}
		}
		backup = WorkFolder(AppendFileName(picker_source, "build/backups"), "catalogue");
		job.Status("Backing up the current generated catalogue, then installing the prepared files.");
		for(int i = 0; i < files.GetCount(); ++i) {
			String name = AsString(files.GetKey(i)); String old = AppendFileName(target, name);
			if(FileExists(old)) { if(!FileCopy(old, AppendFileName(backup, name))) throw Exc("Could not back up " + name); }
			else created.Add(name);
		}
		Write(AppendFileName(backup, "UiSymbolPicker.upp"), package_before);
		// Cancellation stops at the transaction boundary; installation/rollback is indivisible.
		job.Check();
		for(int i = 0; i < files.GetCount(); ++i) {
			String name = AsString(files.GetKey(i)); installed.Add(name);
			if(!FileCopy(AppendFileName(stage, name), AppendFileName(target, name))) throw Exc("Could not install " + name);
		}
		if(package_after != package_before) { package_written = true; Write(package, package_after); }
		result.report.Add("backup", backup); result.report.Add("installed", target); result.report.Add("commit", Get(receipt, "commit"));
		WriteJson(AppendFileName(backup, "installation.json"), result.report);
		result.summary = "Refresh installed in the source folder. Build Symbol Picker with Build.ps1, then open the new executable. The running catalogue stays unchanged until you rebuild. Your previous files are backed up at " + backup;
		result.ok = true;
	}
	catch(const Exc& error) {
		String rollback_errors;
		if(package_written && !FileCopy(AppendFileName(backup, "UiSymbolPicker.upp"), package)) rollback_errors << "\nCould not restore " << package;
		for(const auto& name : installed) {
			String old = AppendFileName(backup, name), path = AppendFileName(target, name);
			bool restored = FileExists(old) ? FileCopy(old, path) : FileDelete(path);
			if(!restored) rollback_errors << "\nCould not restore " << path;
		}
		result.error = String(error) + rollback_errors;
	}
}

void ScanSymbolPickerUsage(const String& project, const String& code_root,
                          const String& output, SymbolPickerMaintenanceJob& job,
                          SymbolPickerMaintenanceResult& result)
{
	try {
		job.Check(); result.project = GetFullPath(project); result.project_before = LoadFile(project);
		ValueMap root = ReadJson(project); Value collections = Get(root, "collections");
		if(!IsValueArray(collections)) throw Exc("Choose a saved Symbol Picker project with collections.");
		if(!DirectoryExists(code_root)) throw Exc("Choose the application's source-code folder.");
		ValueArray groups(collections); Index<String> aliases;
		for(Value group : groups) {
			if(!IsValueMap(group) || !IsValueArray(Get(ValueMap(group), "items"))) throw Exc("Invalid collection in the project.");
			for(Value item : ValueArray(Get(ValueMap(group), "items"))) {
				if(!IsValueMap(item)) throw Exc("Invalid icon in the project.");
				String alias = AsString(Get(ValueMap(item), "alias")); if(!alias.IsEmpty()) aliases.FindAdd(alias);
			}
		}
		Vector<Vector<String>> hits; hits.SetCount(aliases.GetCount());
		Vector<String> files; CodeFiles(GetFullPath(code_root), files, job); Sort(files);
		ValueArray errors, report_items;
		String base = AsString(Get(root, "output_base_name")); int scanned = 0;
		for(const auto& path : files) {
			job.Check(); String name = GetFileName(path);
			if(ToLower(GetFullPath(path)) == ToLower(result.project) || name.EndsWith(".uppicons.json") || name == "UiIcons.h" || name == "UiIcons.cpp" || name == "UiIcons.iml" || name == base + ".h" || name == base + ".cpp" || name == base + ".iml") continue;
			job.Status(Format("Checking source file %d of %d: ", ++scanned, files.GetCount()) + name);
			String text = LoadFile(path);
			if(text.IsVoid()) { errors.Add(path); continue; }
			text = ScanText(text); Index<int> found;
			for(int pos = 0; pos < text.GetCount();) {
				if(IsAlpha((byte)text[pos]) || text[pos] == '_') {
					int start = pos++;
					while(pos < text.GetCount() && (IsAlNum((byte)text[pos]) || text[pos] == '_')) ++pos;
					int index = aliases.Find(text.Mid(start, pos - start)); if(index >= 0) found.FindAdd(index);
				}
				else if(IsDigit((byte)text[pos])) { ++pos; while(pos < text.GetCount() && (IsAlNum((byte)text[pos]) || text[pos] == '_')) ++pos; }
				else ++pos;
			}
			for(int index : found) hits[index].Add(path);
		}
		int matched = 0;
		for(int g = 0; g < groups.GetCount(); ++g) {
			ValueMap group(groups[g]); ValueArray items(Get(group, "items"));
			for(int i = 0; i < items.GetCount(); ++i) {
				ValueMap item(items[i]); String alias = AsString(Get(item, "alias")); int index = aliases.Find(alias);
				ValueArray matches;
				if(index >= 0) for(const auto& file : hits[index]) matches.Add(file);
				if(!matches.IsEmpty()) {
					++matched; item.Set("marked_used", true);
					String note = AsString(Get(item, "usage_note", String())); int old = note.Find("\nScan evidence:");
					if(old >= 0) note.Trim(old); else if(note.StartsWith("Scan evidence:")) note.Clear();
					String evidence = "Scan evidence: " + Join(hits[index], "; ");
					item.Set("usage_note", note.IsEmpty() ? evidence : note + "\n" + evidence);
				}
				ValueMap row; row.Add("alias", alias); row.Add("catalog_id", Get(item, "catalog_id")); row.Add("marked_used", Get(item, "marked_used", false)); row.Add("matches", matches); report_items.Add(row);
				items.Set(i, item);
			}
			group.Set("items", items); groups.Set(g, group);
		}
		root.Set("collections", groups); result.project_after = AsJSON(root, true) + "\n";
		result.report.Add("project", result.project); result.report.Add("code_root", GetFullPath(code_root)); result.report.Add("files_scanned", scanned); result.report.Add("matched_items", matched); result.report.Add("items", report_items); result.report.Add("read_errors", errors);
		result.report.Add("method", "Literal identifier scan, including comments/inactive code and quoted references. Excludes generated directories, manifests and icon definitions. Dynamic names and external callers can be missed. Existing marks and manual notes are preserved; no marks are cleared.");
		String folder = WorkFolder(output, "usage"); String report = AppendFileName(folder, "usage.json"); result.report.Add("report_path", report); WriteJson(report, result.report);
		result.summary = Format("%d collection items; %d with explicit references in %d source files. %d unreadable files. Review the evidence before applying marks. No existing marks will be cleared.", report_items.GetCount(), matched, scanned, errors.GetCount());
		if(!errors.IsEmpty()) throw Exc("Some files could not be read. Fix those files and scan again before applying marks.");
		job.Check(); result.ok = true;
	}
	catch(const Exc& error) { result.error = error; }
}

void ApplySymbolPickerUsage(const SymbolPickerMaintenanceResult& scan, const String& output,
                           SymbolPickerMaintenanceJob& job, SymbolPickerMaintenanceResult& result)
{
	try {
		job.Check();
		if(!scan.ok || scan.project_after.IsEmpty()) throw Exc("Run a successful usage scan first.");
		if(LoadFile(scan.project) != scan.project_before) throw Exc("The project changed after the scan. Save it and scan again before applying marks.");
		String backup = AppendFileName(WorkFolder(output, "usage-backup"), GetFileName(scan.project));
		Write(backup, scan.project_before);
		job.Check();
		String temporary = scan.project + ".tmp-" + FormatInt64((int64)Random64());
		Write(temporary, scan.project_after);
#ifdef PLATFORM_WIN32
		bool replaced = !!ReplaceFileW(ToSystemCharsetW(scan.project), ToSystemCharsetW(temporary), NULL, 0, NULL, NULL);
#else
		bool replaced = FileMove(temporary, scan.project);
#endif
		if(!replaced) { FileDelete(temporary); throw Exc("Could not replace the project. The original and backup are intact."); }
		result.report.Add("backup", backup); result.report.Add("project", scan.project);
		result.summary = "Usage warnings saved. Existing marks and manual notes were preserved. Reload this project in Symbol Picker to see the marks. Backup: " + backup;
		result.ok = true;
	}
	catch(const Exc& error) { result.error = error; }
}

}
