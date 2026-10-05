#include "SymbolPickerHelp.h"

namespace Upp {

namespace {

void PopulateHelp(UiDocCore& model)
{
	WString text;
	Vector<UiDocBlock> blocks;
	auto Add = [&](const char* value, const char* role) {
		UiDocBlock& block = blocks.Add();
		block.range.from = text.GetCount();
		text << String(value).ToWString();
		block.range.to = text.GetCount();
		block.role = role;
		text.Cat('\n');
	};
	auto Heading = [&](const char* title) { Add(title, "heading.2"); };
	auto Paragraph = [&](const char* value) { Add(value, "paragraph"); };
	auto Bullet = [&](const char* value) { Add(value, "list.bullet"); };
	Heading("Why Symbol Picker exists");
	Paragraph("Symbol Picker helps you quickly find icons, try them together, and maintain a named icon set for an application. It grew from browsing the symbol catalogue into keeping reusable collections: choose a set once, save its project, and return whenever the application needs another icon.");
	Paragraph("The project is the editable source of your choices. Exported headers and images are build assets. Keep application-specific icons in your application's own set, alongside the shared Ui icons used by common controls.");
	Heading("Browse and choose symbols");
	Bullet("Choose a category. The Library heading shows the category and the number of matching symbols. Each category has a stable colour. Filter categories by name, or search the Library by symbol name.");
	Bullet("Icons / SVG and Images / PNG choose separate library views. The Material Image category contains SVG icons about images; actual PNG/JPEG files are in the Images / PNG view. Outlined, Rounded and Sharp change Material artwork. Custom SVGs and shared Ui icons do not have three interchangeable styles.");
	Bullet("Tint chooses the colour saved with icons you add. Transparent/empty tint preserves source colours. Dark mode uses white previews for visibility; it does not change your saved tint or export settings.");
	Bullet("Use + and minus, enter a percentage and press Enter, or hold Ctrl while rolling the wheel to zoom. Zoom changes the preview layout, not the export resolution.");
	Bullet("Click to select one icon. Ctrl+click toggles individual selections; Shift+click selects a range. Ctrl+A selects all in the focused gallery. Drag from empty space to select a rectangle. Alt is not a selection shortcut.");
	Bullet("Double-click a library icon to add it, or drag the selected icons into the active collection. Duplicate catalogue IDs in that collection are skipped. Drag a selected icon without Ctrl/Shift to move the current selection.");
	Heading("Collections: your application's icon set");
	Bullet("The collection dropdown selects the application set being edited, such as UiIcons or MyApp. + creates a collection; minus removes the active collection. The supplied UiIcons projects keep the whole application set together; source categories stay on the items. All assets / Icons / SVG / Images / PNG changes only the collection view. Icons and raster images are saved together in the same project.");
	Bullet("Drag collection tiles to reorder them. Delete removes selected collection items; Clear removes all items in the active collection. Removing a tile does not remove it from the source catalogue.");
	Bullet("Friendly tile names help you browse; each item also has a stable export alias, size and tint. Keep aliases stable after using them in code. Existing Ui entries preserve their public ICON_* names. Advanced metadata and aliases can be edited in the saved JSON; there is not yet a full item-properties inspector.");
	Bullet("Select collection tiles and choose Mark used or Unmark used. A blue dot marks a usage warning; an orange dot marks a missing source. Deleting, clearing or removing a collection containing marked items asks you to confirm. This is a warning, not a deletion lock.");
	Bullet("A usage mark is recorded on the collection item. It may be set manually or by a code scan, with a usage_note listing evidence. A scan finds explicit names; dynamically built names, external callers and generated code may need manual review. An unmarked item is not proof that it is unused.");
	Bullet("Ctrl+Z undoes collection actions and usage marks; Ctrl+Y redoes them. An asterisk next to the collection name indicates unsaved changes.");
	Heading("Save, load and return later");
	Bullet("Save writes the entire project, including every collection, to .symbolpicker. Save As chooses another path. The file records catalogue IDs, aliases, sizes, tints, category overrides and usage notes; it does not embed the source artwork.");
	Bullet("Store the project beside your application's icon assets, for example AppIcons.symbolpicker. Load reopens that project. If a source is missing it remains visible as unresolved, and exporters report or skip it instead of inventing an image.");
	Bullet("Saving a project does not export icons. After adding or changing icons, Save, export again to the same application asset names, then rebuild your application. No automatic file watcher updates the running application.");
	Heading("Export format: what each option produces");
	Bullet("Image Call: a list of the generated function names. It is a naming reference, not embedded artwork; the functions come from your generated header. Add () when calling a function.");
	Bullet("Icon Id: source catalogue IDs plus aliases/categories, useful for inventories or tools that already understand those IDs. An ID alone is not an image.");
	Bullet("C++ Snippet: a metadata table of names, source IDs, categories, sizes and comments. It is not an image implementation.");
	Bullet("PNG Files: transparent raster files, arranged by collection. Useful for other toolkits, design review or image pipelines.");
	Bullet("SVG Files: original vector artwork with export size/tint applied. Existing Ui raster/custom factories have no SVG payload and are skipped with warnings; PNG or IML is appropriate for those entries.");
	Bullet("U++ RAW Header: a standalone generated header with pixel data and named image factories. Useful when a package should not carry an IML file, at the cost of larger generated source.");
	Bullet("U++ RLE Header: the same standalone interface with run-length-compressed image data. Compression changes storage, not visual quality.");
	Bullet("U++ IML: standard U++ image-list data. Your package supplies its normal IMAGECLASS declarations and implementation.");
	Bullet("U++ IML + Header: the recommended U++ application option. Writes matching .iml and .h files; the header declares the image class, named image functions, a catalogue (category/name/factory), and a category list. Define its documented *_IML_IMPLEMENTATION macro in exactly one .cpp before including the header.");
	Bullet("Export writes the active collection. The split arrow offers Export all for the whole project. Copy copies text formats to the clipboard; file-based formats require Export. Check any _export_warnings.txt before integrating outputs.");
	Heading("Size, sharpness and a consistent interface");
	Bullet("The size dropdown controls the pixel size captured when adding an icon; each existing item retains its own size. Changing this dropdown does not resize existing collection items. Edit item size in JSON when migrating an existing set.");
	Bullet("A good starting point for this Ui workflow is a 48x48 asset canvas, displayed at 20 or 24 logical pixels in ordinary buttons. Use 16 for dense menus, and 32 for prominent tools or previews. Keep the clickable button larger than the icon.");
	Bullet("For a 24-logical-pixel icon, 48 physical pixels cover 200% scaling. For 32 logical pixels at 200%, generate 64. A 60-pixel source is possible but 64 is a more useful standard step; actual targets depend on display scale and control size.");
	Bullet("Vector originals can be rendered cleanly at the target size. Enlarging a 38-pixel raster to 48 or 64 adds pixels without recovering detail. Some current Ui assets are cropped to their painted bounds, so an _48 suffix is not proof of a 48x48 stored image.");
	Bullet("Judge the visible silhouette, padding and stroke weight at its actual displayed size, in light and dark themes and at common DPI scales. A larger source alone will not fix an icon that is drawn too small inside its canvas.");
	Heading("A repeatable application workflow");
	Bullet("Start a project for the application, group icons into collections, and choose stable aliases. Save AppIcons.symbolpicker in the application package and commit it alongside the source.");
	Bullet("Export all as IML + Header to AppIcons.h/AppIcons.iml, keeping those names separate from UiIcons. Add AppIcons.cpp with the generated implementation macro once; include the header where needed.");
	Bullet("Use the generated catalogue/category functions if the application needs a list of available icons. This is a plain C++ catalogue; it does not automatically register the set into the shared Ui designer catalogue.");
	Bullet("Mark icons as used as they enter code. Before removing one, scan callers and review any usage note. Reopen the saved project, change the collection, re-export to the same files and rebuild.");
	Bullet("The shared UiIcons project is an inventory and maintenance starting point. Its current raster/custom assets are preserved as sources. Review generated names, image-class integration and image quality before replacing the shared UiIcons header or IML; the existing header also has hand-written lookup helpers.");
	model.Replace(UiDocRange(0, model.GetLength()), text);
	for(const auto& block : blocks) model.AddBlock(block.range, block.role);
}

// UiDoc is an editor; Help permits navigation/selection/copy only.
class HelpDocument : public UiDoc {
public:
	bool Key(dword key, int count) override {
		if(key == K_CTRL_C || key == K_CTRL_A) return UiDoc::Key(key, count);
		dword navigation = key & ~(K_CTRL | K_SHIFT);
		if(navigation == K_LEFT || navigation == K_RIGHT || navigation == K_UP || navigation == K_DOWN
		|| navigation == K_HOME || navigation == K_END || navigation == K_PAGEUP || navigation == K_PAGEDOWN)
			return UiDoc::Key(key, count);
		if(key == K_TAB || key == (K_SHIFT | K_TAB) || key == K_ESCAPE) return false;
		return true;
	}
};

class SymbolPickerHelpWindow : public TopWindow {
public:
	SymbolPickerHelpWindow() {
		Ctrl::SkinChangeSensitive();
		Title("Symbol Picker — Help").Sizeable().Zoomable();
		SetRect(0, 0, DPI(820), DPI(760));
		SetMinSize(Size(DPI(600), DPI(460)));
		root_.SetGap(DPI(10)).SetInset(DPI(14));
		Add(root_.SizePos());
		title_.SetTitle("Symbol Picker").SetSubTitle("Find, collect and maintain icons for your applications")
		      .SetSelectable(false).EnableHover(false);
		root_.Add(title_).Fit();
		PopulateHelp(text_.Model());
		text_.SetSelection(UiDocRange(0, 0));
		text_.ShowMetadataMarkers(false);
		ApplyTheme();
		root_.Add(text_).Expand(1);
		close_.SetText("Close");
		close_.WhenAction = [=] { Close(); };
		root_.Add(close_).Fit().AlignSelf(UiBoxLayout::Align::End);
	}
	void ApplyTheme() {
		title_.SetCustomStyle(UiTheme::ResolveTitleCard());
		auto style = UiTheme::ResolveDoc();
		auto surface = UiTheme::ResolvePanel();
		auto edit = UiTheme::ResolveEdit();
		style.palette = surface.palette;
		for(int state = 0; state < 4; ++state) style.palette.ink[state] = edit.palette.ink[state];
		style.skin = StyledSkin();
		style.metrics.radius = DPI(8);
		style.page_face = surface.palette.face[ST_NORMAL].color;
		style.page_frame = surface.palette.frame[ST_NORMAL];
		style.font = SansSerifZ(11);
		style.paragraph_gap = DPI(8);
		style.page_padding = DPI(18);
		style.caret_ink = edit.caret_color;
		style.selection_fill = edit.selection_color;
		text_.SetCustomStyle(style);
	}
	void Skin() override { ApplyTheme(); }
private:
	UiBoxLayout root_ { UiDirection::V };
	UiTitleCard title_;
	HelpDocument text_;
	UiButton close_;
};

}

void ShowSymbolPickerHelp(One<TopWindow>& window)
{
	if(window && window->IsOpen()) { window->SetForeground(); return; }
	window = new SymbolPickerHelpWindow;
	window->OpenMain();
}

}
