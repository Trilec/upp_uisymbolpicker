#include "SymbolPickerWorkspaceView.h"
#include "SymbolPickerHelp.h"
#include "Utilities/SymbolPickerUtilities.h"

namespace Upp {

namespace {

static constexpr int kPreviewPx = 24;



UiLabel::Style HeadingStyle()
{
	UiLabel::Style style = UiTheme::ResolveLabel(UiRole::Accent);
	style.font = SansSerifZ(12).Bold();
	return style;
}

UiLabel::Style StatusStyle()
{
	UiLabel::Style style = UiTheme::ResolveLabel(UiRole::Subtle);
	style.font = SansSerifZ(9);
	return style;
}

Color CategoryTint(int index, int count)
{
	const auto t = UiTheme::GetOliveTokens(UiTheme::GetMode());
	return Blend(Blend(t.panel, t.accent, 66), Blend(t.panel, t.muted, 24),
	             count > 1 ? 255 * index / (count - 1) : 0);
}

const char* IconStyleText(SymbolPickerIconStyle style)
{
	switch(style) {
	case SymbolPickerIconStyle::Rounded: return "Rounded";
	case SymbolPickerIconStyle::Sharp: return "Sharp";
	case SymbolPickerIconStyle::Outlined:
	default: return "Outlined";
	}
}

String SafeAliasPart(const String& text)
{
	String out;
	for(int i = 0; i < text.GetCount(); ++i) {
		int c = (byte)text[i];
		if(IsAlNum(c))
			out.Cat(ToUpper((wchar)c));
        else if(out.IsEmpty() || out[out.GetCount() - 1] != '_')
			out.Cat('_');
	}
    while(!out.IsEmpty() && out[out.GetCount() - 1] == '_')
		out.Trim(out.GetCount() - 1);
	return out;
}

bool MatchText(const String& haystack, const String& filter)
{
	String needle = ToLower(TrimBoth(filter));
	return needle.IsEmpty() || ToLower(haystack).Find(needle) >= 0;
}

bool PromptCollectionName(String& name)
{
	TopWindow dialog; dialog.Title("New application collection"); dialog.SetRect(0, 0, DPI(460), DPI(220));
	UiBoxLayout root(UiDirection::V), buttons(UiDirection::H);
	UiTitleCard title; UiLineEdit edit; UiButton create, cancel;
	root.SetInset(DPI(16)).SetGap(DPI(12)); dialog.Add(root.SizePos());
	title.SetTitle("New collection").SetSubTitle("Keep one application's icons and images together").SetSelectable(false);
	edit.SetData(name); edit.SetPlaceholder("Application or collection name");
	create.SetText("Create"); cancel.SetText("Cancel");
	create.WhenAction = [&] { if(!TrimBoth(AsString(edit.GetData())).IsEmpty()) dialog.AcceptBreak(IDOK); };
	edit.WhenAction = create.WhenAction; cancel.WhenAction = [&] { dialog.Break(IDCANCEL); };
	buttons.SetGap(DPI(10)); buttons.AddSpacer(1).Expand(1); buttons.Add(cancel).Fit(); buttons.Add(create).Fit();
	root.Add(title).Fit(); root.Add(edit).Fixed(DPI(32)); root.AddSpacer(1).Expand(1); root.Add(buttons).Fixed(DPI(34));
	edit.SetFocus(); if(dialog.Run() != IDOK) return false;
	name = TrimBoth(AsString(edit.GetData())); return !name.IsEmpty();
}

} // namespace

SymbolPickerWorkspaceDragPreview::SymbolPickerWorkspaceDragPreview()
{
	IgnoreMouse();
	NoWantFocus();
	Hide();
}

void SymbolPickerWorkspaceDragPreview::SetPreview(const Image& image, int count)
{
	image_ = image;
	count_ = max(1, count);
	Refresh();
}

void SymbolPickerWorkspaceDragPreview::Paint(Draw& w)
{
	Rect r = GetSize();
	if(r.IsEmpty())
		return;
	const auto t = UiTheme::GetOliveTokens(UiTheme::GetMode());
	Color frame = t.accent;
	w.DrawRect(r, t.panel);
	w.DrawRect(r.left, r.top, r.GetWidth(), 1, frame);
	w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, frame);
	w.DrawRect(r.left, r.top, 1, r.GetHeight(), frame);
	w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), frame);
	if(!image_.IsEmpty())
		w.DrawImage(DPI(8), DPI(8), DPI(32), DPI(32), image_);
	if(count_ > 1)
		w.DrawText(DPI(46), DPI(17), Format("%d items", count_), SansSerifZ(9).Bold(), t.text);
}

Size SymbolPickerWorkspaceDragPreview::GetMinSize() const
{
	return Size(DPI(112), DPI(48));
}

SymbolPickerWorkspaceView::SymbolPickerWorkspaceView()
{
	Ctrl::SkinChangeSensitive();
	Title("Symbol Picker");
	Sizeable().Zoomable();
	SetRect(0, 0, DPI(1240), DPI(840));
	SetMinSize(Size(DPI(960), DPI(680)));
	BuildUi();
	ConfigureGalleryRenderers();
	WireActions();
	ui_ready_ = true;
	UpdateThemeButton();
}

void SymbolPickerWorkspaceView::BuildUi()
{
	root_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
	Add(root_.SizePos());

	BuildHeader();
	BuildCategories();
	BuildLibrary();
	BuildCollections();

	root_.Add(header_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	root_.Add(categories_panel_).Fixed(DPI(142)).MinMain(DPI(120)).AlignSelf(UiBoxLayout::Align::Stretch);
	library_collection_split_.Vert(library_panel_, collections_panel_)
		.SetSplitPercent(60).SetMinPixels(0, DPI(170)).SetMinPixels(1, DPI(170))
		.SetGripVisual(UISPLITTER_GRIP_DOTS);
	library_collection_split_.Tip("Drag to give the library or collection more space");
	root_.Add(library_collection_split_).Expand(1).MinMain(DPI(348)).AlignSelf(UiBoxLayout::Align::Stretch);

	Add(drag_preview_);
}

void SymbolPickerWorkspaceView::BuildHeader()
{
	header_.SetDirection(UiDirection::H).SetGap(DPI(15)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	app_title_.SetCustomStyle(HeadingStyle()).SetText("Symbol Picker");
	project_label_.SetCustomStyle(StatusStyle()).SetText("Model-driven icon workspace");
	theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("theme");
	utilities_button_.SetText("Utilities").Tip("Compare/download the symbol catalogue, or scan collection usage");
	help_button_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("help");
	exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("exit");

	header_.Add(app_title_).Fit().MinMain(DPI(120));
	header_.Add(project_label_).Fit().MinMain(DPI(220));
	header_.AddSpacer(1).Expand(1).MinMain(DPI(10));
	header_.Add(theme_button_).Fixed(DPI(34));
	header_.Add(utilities_button_).Fit();
	header_.Add(help_button_).Fixed(DPI(34));
	header_.Add(exit_button_).Fixed(DPI(34));
}

void SymbolPickerWorkspaceView::BuildCategories()
{
	categories_panel_.Add(categories_box_.SizePos());
	categories_box_.SetDirection(UiDirection::V).SetGap(DPI(5)).SetInset(DPI(7));
	categories_header_.SetDirection(UiDirection::H).SetGap(DPI(15)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	categories_title_.SetCustomStyle(HeadingStyle()).SetText("Categories");
	symbols_view_.SetText("Icons / SVG").SetCheckable(true).SetChecked(true);
	images_view_.SetText("Images / PNG").SetCheckable(true);
	images_view_.Tip("Raster images: PNG, JPEG, BMP and GIF. Separate from the Material Image icon category.");
	categories_filter_.SetPlaceholder("Filter categories");

	categories_header_.Add(categories_title_).Fit().MinMain(DPI(120));
	categories_header_.Add(symbols_view_).Fit();
	categories_header_.Add(images_view_).Fit();
	categories_header_.AddSpacer(1).Expand(1).MinMain(DPI(8));
	categories_header_.Add(categories_filter_).Fit().MinMain(DPI(190));

	categories_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
	categories_scroll_.Content().Add(categories_flow_.SizePos());
	categories_flow_.SetDirection(UiDirection::H)
	                .SetGap(DPI(4), DPI(4))
	                .SetInset(0)
	                .SetWrap(UiBoxWrap::Flow)
	                .SetWrapAutoResize(true)
	                .SetFixedColumn(DPI(188));

	categories_box_.Add(categories_header_).Fit();
	categories_box_.Add(categories_scroll_).Expand(1);
}

void SymbolPickerWorkspaceView::BuildLibrary()
{
	library_panel_.Add(library_box_.SizePos());
	library_box_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(7));
	library_header_.SetDirection(UiDirection::H).SetGap(DPI(15)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	library_actions_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0)
	                .SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true).SetAlignItems(UiCrossAlign::Center);

	library_title_.SetCustomStyle(HeadingStyle()).SetText("Library");
	library_status_.SetCustomStyle(StatusStyle()).SetText("No catalog loaded");
	library_header_.Add(library_title_).Fit().MinMain(DPI(320));
	library_header_.Add(library_status_).Expand(1).MinMain(DPI(220));

	library_style_.UseInternalModel().Clear()
		.Add("Outlined", (int)SymbolPickerIconStyle::Outlined)
		.Add("Rounded", (int)SymbolPickerIconStyle::Rounded)
		.Add("Sharp", (int)SymbolPickerIconStyle::Sharp);
	library_style_.SetSizeMin(DPI(118), 0);
	library_optical_.Add("20 optical", 20).Add("48 optical", 48).SetDataSilently(20);
	library_optical_.Tip("Optical design changes stroke and detail; export pixel size stays independent");
	library_filled_.SetText("Filled");
	tint_label_.SetText("Tint");
	library_tint_ <<= Black();
	library_tint_.NullText("Auto");
	library_tint_.Tip("Foreground tint; Auto uses the theme's icon colour");
	background_label_.SetText("Background");
	preview_background_ <<= Color(Null);
	preview_background_.NullText("Auto");
	preview_background_.Tip("Tile background in both galleries; Auto follows the theme. Preview only; exports stay transparent.");
	library_zoom_in_.SetText("+").Tip("Zoom in");
	library_zoom_out_.SetText("−").Tip("Zoom out");
	library_zoom_.SetTextUtf8("100%").Tip("Zoom percentage; press Enter to apply");
	library_zoom_hint_.SetCustomStyle(StatusStyle()).SetText("Ctrl + wheel to zoom");
	library_filter_.SetPlaceholder("Search symbols");
	library_include_metadata_.SetText("Include metadata").SetChecked(true);
	library_include_metadata_.Tip("Include embedded Google search tags; turn off to match symbol names and IDs only");
	import_assets_.SetText("Import…").Tip("Add SVG or image files to the library; the project keeps file references.");

	library_actions_.Add(library_style_).Fit().MinMain(DPI(118));
	library_actions_.Add(library_optical_).Fixed(DPI(105));
	library_actions_.Add(library_filled_).Fit();
	library_actions_.Add(tint_label_).Fit();
	library_actions_.Add(library_tint_).Fixed(DPI(42)).MinCross(DPI(24));
	library_actions_.Add(background_label_).Fit();
	library_actions_.Add(preview_background_).Fixed(DPI(42)).MinCross(DPI(24));
	library_actions_.Add(library_zoom_in_).Fixed(DPI(30));
	library_actions_.Add(library_zoom_).Fixed(DPI(64));
	library_actions_.Add(library_zoom_out_).Fixed(DPI(30));
	library_actions_.Add(library_zoom_hint_).Fit().MinMain(DPI(145));
	library_actions_.Add(import_assets_).Fixed(DPI(76));
	library_actions_.AddSpacer(1).Expand(1).MinMain(DPI(8));
	library_actions_.Add(library_filter_).Expand(1).MinMain(DPI(220));
	library_actions_.Add(library_include_metadata_).Fit();

	library_gallery_.SetDragSource(SymbolPickerGestureSourceKind::Library)
	                .SetModel(library_projection_.Model())
	                .SetSelectionMode(UIGALLERYSEL_MULTI)
	                .SetItemSize(Size(DPI(64), DPI(56)))
	                .SetGap(DPI(4))
	                .SetInset(DPI(6))
	                .SetOverscanRows(2)
	                .SetZoomRange(0.62, 1.75, 1.12);

	library_box_.Add(library_header_).Fit();
	library_box_.Add(library_actions_).Fit();
	library_box_.Add(library_gallery_).Expand(1);
}

void SymbolPickerWorkspaceView::BuildCollections()
{
	collections_panel_.Add(collections_box_.SizePos());
	collections_box_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(7));
	collections_header_.SetDirection(UiDirection::H).SetGap(DPI(15)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	collections_actions_.SetDirection(UiDirection::H).SetGap(DPI(6)).SetInset(0)
	                    .SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true).SetAlignItems(UiCrossAlign::Center);

	collections_title_.SetCustomStyle(HeadingStyle()).SetText("Collections");
	collections_status_.SetCustomStyle(StatusStyle()).SetText("No active collection");
	collections_header_.Add(collections_title_).Fit().MinMain(DPI(110));
	collections_header_.Add(collections_status_).Expand(1).MinMain(DPI(220));

	collections_selector_.SetSizeMin(DPI(180), 0);
	new_collection_.SetText("+");
	new_collection_.Tip("Create collection");
	remove_collection_.SetText("−");
	remove_collection_.Tip("Remove active collection");

	save_button_.SetText("Save").SetSplitWidth(DPI(26)).SetPopupMinWidth(DPI(150));
	save_button_.ClearItems().Add("Save", "save").Add("Save As", "save_as");
	load_button_.SetText("Load");

	export_button_.SetText("Export").SetSplitWidth(DPI(26)).SetPopupMinWidth(DPI(160));
	export_button_.ClearItems().Add("Export current", "current").Add("Export all", "all");

	export_size_.UseInternalModel().Clear()
		.Add("16 px", 16).Add("24 px", 24).Add("32 px", 32).Add("48 px", 48)
		.Add("64 px", 64).Add("128 px", 128).Add("256 px", 256).Add("512 px", 512);
	export_size_.SetSizeMin(DPI(92), 0);

	export_type_.UseInternalModel().Clear()
		.Add("Image Call", (int)SymbolPickerExportType::ImageCall)
		.Add("Icon Id", (int)SymbolPickerExportType::IconId)
		.Add("C++ Snippet", (int)SymbolPickerExportType::CppSnippet)
		.Add("PNG Files", (int)SymbolPickerExportType::PngFiles)
		.Add("SVG Files", (int)SymbolPickerExportType::SvgFiles)
		.Add("U++ RAW Header", (int)SymbolPickerExportType::UppRawHeader)
		.Add("U++ RLE Header", (int)SymbolPickerExportType::UppRleHeader)
		.Add("U++ IML", (int)SymbolPickerExportType::UppIml)
		.Add("U++ IML + Header", (int)SymbolPickerExportType::UppImlLibrary);
	export_type_.SetSizeMin(DPI(170), 0);

	copy_button_.SetText("Copy");
	copy_button_.Tip("Copy current text export");
	delete_items_.SetText("Delete");
	delete_items_.Tip("Remove selected collection items");
	mark_used_.SetText("Mark used").Tip("Toggle the usage warning for selected icons; blue dot means marked used");
	clear_collection_.SetText("Clear");
	clear_collection_.Tip("Clear active collection");
	collection_filter_.SetPlaceholder("Filter collection");

	collections_actions_.Add(collections_selector_).Fit().MinMain(DPI(180));
	collections_actions_.Add(new_collection_).Fixed(DPI(34));
	collections_actions_.Add(remove_collection_).Fixed(DPI(34));
	collections_actions_.Add(save_button_).Fixed(DPI(86));
	collections_actions_.Add(load_button_).Fixed(DPI(70));
	collections_actions_.Add(export_button_).Fixed(DPI(92));
	collections_actions_.Add(export_size_).Fit().MinMain(DPI(92));
	collections_actions_.Add(export_type_).Fit().MinMain(DPI(170));
	collections_actions_.Add(copy_button_).Fixed(DPI(66));
	collections_actions_.Add(delete_items_).Fixed(DPI(72));
	collections_actions_.Add(mark_used_).Fixed(DPI(90));
	collections_actions_.Add(clear_collection_).Fixed(DPI(64));
	collections_actions_.Add(collection_filter_).Expand(1).MinMain(DPI(170));

	collection_gallery_.SetDragSource(SymbolPickerGestureSourceKind::Collection)
	                   .SetModel(collection_projection_.Model())
	                   .SetSelectionMode(UIGALLERYSEL_MULTI)
	                   .SetItemSize(Size(DPI(64), DPI(56)))
	                   .SetGap(DPI(4))
	                   .SetInset(DPI(6))
	                   .SetOverscanRows(2)
	                   .SetZoomRange(0.72, 1.55, 1.12);

	collections_box_.Add(collections_header_).Fit();
	collection_asset_view_.Add("All assets", -1).Add("Icons / SVG", 0).Add("Images / PNG", 1).SetDataSilently(-1);
	collections_header_.Add(collection_asset_view_).Fixed(DPI(150));
	collections_box_.Add(collections_actions_).Fit();
	collections_box_.Add(collection_gallery_).Expand(1);
}

void SymbolPickerWorkspaceView::ConfigureGalleryRenderers()
{
	const auto t = UiTheme::GetOliveTokens(UiTheme::GetMode());
	Color background = (Color)~preview_background_;
	if(IsNull(background)) background = t.panel;
	Color label_ink = IsNull((Color)~preview_background_) ? t.text
		: (background.GetR() * 299 + background.GetG() * 587 + background.GetB() * 114 > 145000
		   ? UiTheme::GetOliveTokens(UiThemeMode::Light).text : UiTheme::GetOliveTokens(UiThemeMode::Dark).text);
	UiItemRenderImage library_render;
	UiItemRenderStyle library_style = library_render.GetStyle();
	library_style.image_extent = DPI(24);
	library_style.title_font = SansSerifZ(8);
	library_style.skin = StyledSkin();
	library_style.metrics.content_margin = Rect(DPI(3), DPI(3), DPI(3), DPI(4));
	library_style.metrics.radius = DPI(4);
	library_style.metrics.face_enabled = true;
	library_style.metrics.frame_enabled = true;
	library_style.metrics.frame_width = 1;
	library_style.metrics.shadow.enabled = false;
	library_style.metrics.highlight.enabled = false;
	for(int state = 0; state < 4; ++state) {
		library_style.palette.face[state] = UiFill::Solid(background);
		library_style.palette.frame[state] = t.line_soft;
		library_style.palette.ink[state] = label_ink;
	}
	library_style.palette.face[ST_HOT] = UiFill::Solid(Blend(background, t.accent, 12));
	library_style.palette.face[ST_PRESSED] = UiFill::Solid(Blend(background, t.accent, 30));
	library_style.palette.frame[ST_HOT] = t.line;
	library_style.palette.frame[ST_PRESSED] = t.accent;
	library_style.palette.ink[ST_DISABLED] = t.disabled_ink;
	library_style.metadata_default = t.accent;
	library_style.show_description = false;
	library_style.show_right_text = false;
	library_style.show_metadata = false;
	library_style.content_gap = DPI(4);
	library_render.SetCustomStyle(library_style);
	library_gallery_.SetItemRender(library_render);

	UiItemRenderImage collection_render;
	library_style.show_metadata = true;
	collection_render.SetCustomStyle(library_style);
	collection_gallery_.SetItemRender(collection_render);
	for(SymbolPickerDragGallery* gallery : { &library_gallery_, &collection_gallery_ }) {
		gallery->ClearCustomStyle();
		auto style = gallery->GetStyle();
		style.metrics.radius = DPI(8);
		style.metrics.frame_width = 1;
		style.palette.frame[ST_NORMAL] = t.line_soft;
		style.marquee_fill = t.active;
		style.marquee_frame = style.selection_frame = t.accent;
		style.marquee_frame_width = style.selection_frame_width = 1;
		style.skin = StyledSkin();
		gallery->SetCustomStyle(style);
	}
	for(UiPanel* panel : { &categories_panel_, &library_panel_, &collections_panel_ })
		{
		panel->ClearCustomStyle();
		panel->SetRadius(DPI(8));
	}
	library_collection_split_.ClearCustomStyle();

}

void SymbolPickerWorkspaceView::ApplyThemeState()
{
	ConfigureGalleryRenderers();
	app_title_.SetCustomStyle(HeadingStyle());
	categories_title_.SetCustomStyle(HeadingStyle());
	collections_title_.SetCustomStyle(HeadingStyle());
	project_label_.SetCustomStyle(StatusStyle());
	library_status_.SetCustomStyle(StatusStyle());
	collections_status_.SetCustomStyle(StatusStyle());
	library_zoom_hint_.SetCustomStyle(StatusStyle());
	image_cache_.Clear();
	image_style_ = -1;
	seen_library_revision_ = -1;
	seen_collections_revision_ = -1;
	UpdateThemeButton();
	library_gallery_.RefreshLayout();
	collection_gallery_.RefreshLayout();
	root_.RefreshLayout();
	RefreshFromModel();
	Refresh();
}

void SymbolPickerWorkspaceView::Skin()
{
	if(ui_ready_) ApplyThemeState();
}

void SymbolPickerWorkspaceView::Paint(Draw& w)
{
	w.DrawRect(GetSize(), UiTheme::GetOliveTokens(UiTheme::GetMode()).background);
}

void SymbolPickerWorkspaceView::WireActions()
{
	symbols_view_.WhenAction = [=] { SetLibraryAssetKind(0); };
	images_view_.WhenAction = [=] { SetLibraryAssetKind(1); };
	collection_asset_view_.WhenSelectData = [=](const Value&) { RefreshCollectionProjection(false); };
	theme_button_.WhenAction = [=] {
		bool dark = UiTheme::GetMode() != UiThemeMode::Dark;
		UiTheme::Set(dark ? UiThemeMode::Dark : UiThemeMode::Light);
		Ctrl::SwapDarkLight();
		ApplyThemeState();
	};
	file_actions_.WhenCatalogChanged = [=] { SetCatalog(catalog_); };
	import_assets_.WhenAction = [=] { file_actions_.ImportCustomAssets(); };
	library_gallery_.WhenFilesDropped = [=](const Vector<String>& paths) { file_actions_.ImportCustomFiles(paths); };
	collection_gallery_.WhenFilesDropped = [=](const Vector<String>& paths) {
		file_actions_.ImportCustomFiles(paths);
		if(!catalog_) return;
		// Save IDs before callbacks can rebuild the catalogue/projection.
		Vector<String> ids;
		for(const auto& entry : catalog_->GetIcons())
			for(const String& path : paths)
				if(entry.external_file == GetFullPath(path)) ids.Add(entry.catalog_id);
		library_gallery_.ClearSelection();
		for(const String& id : ids) AddLibrarySelectionToCollection(id);
	};
	help_button_.WhenAction = [=] { ShowSymbolPickerHelp(help_window_); };
	utilities_button_.WhenAction = [=] { ShowSymbolPickerUtilities(utilities_window_, model_ ? model_->GetProjectFilePath() : String()); };
	exit_button_.WhenAction = [=] { Close(); };

	categories_filter_.WhenChange = [=] {
		if(!syncing_)
			RefreshCategoryButtons();
	};

	library_style_.WhenAction = [=] {
		if(syncing_ || !model_ || !commands_)
			return;
		Value data = library_style_.GetSelectedData();
		if(!IsNumber(data))
			return;
		SymbolPickerIconStyle style = (SymbolPickerIconStyle)(int)data;
		BeginCommandBatch();
		commands_->Execute(MakeSymbolPickerSetIconStyleCommand(style), *model_);
		if(catalog_ && model_->GetCurrentCategory() != "All" &&
		   catalog_->Filter(model_->GetCurrentCategory(), String(), style, library_asset_kind_, true, model_->GetOpticalSize(), model_->GetFilled()).IsEmpty())
			commands_->Execute(MakeSymbolPickerSetCategoryCommand("All"), *model_);
		EndCommandBatch();
	};
	library_tint_.WhenAction = [=] {
		if(!syncing_ && model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetTintCommand((Color)~library_tint_), *model_);
	};
	preview_background_.WhenAction = [=] { ConfigureGalleryRenderers(); };
	library_optical_.WhenSelectData = [=](const Value&) {
		if(!syncing_ && model_ && commands_) commands_->Execute(MakeSymbolPickerSetMaterialVariantCommand((int)library_optical_.GetData(), library_filled_.IsChecked()), *model_);
	};
	library_filled_.WhenAction = [=] {
		if(!syncing_ && model_ && commands_) commands_->Execute(MakeSymbolPickerSetMaterialVariantCommand((int)library_optical_.GetData(), library_filled_.IsChecked()), *model_);
	};
	library_include_metadata_.WhenAction = [=] { RefreshLibraryProjection(true); UpdateLibraryStatus(); };
	library_filter_.WhenAction = [=] { ApplyLibraryFilter(); };
	library_filter_.WhenChange = [=] {
		if(syncing_)
			return;
		SetTimeCallback(70, [=] { ApplyLibraryFilter(); }, TIMEID_LIBRARY_FILTER);
	};
	library_zoom_in_.WhenAction = [=] { library_gallery_.ZoomBy(1.12); };
	library_zoom_out_.WhenAction = [=] { library_gallery_.ZoomBy(1.0 / 1.12); };
	library_zoom_.WhenAction = [=] {
		String text = TrimBoth(library_zoom_.GetTextUtf8());
		if(text.EndsWith("%")) text.Trim(text.GetCount() - 1);
		const char* end = nullptr;
		double percent = ScanDouble(~text, &end);
		if(!IsNull(percent) && end && *end == 0 && percent > 0)
			library_gallery_.SetZoom(percent / 100.0);
		UpdateLibraryStatus();
	};
	library_gallery_.WhenSelection = [=] { UpdateLibraryStatus(); };
	library_gallery_.WhenZoom = [=](double) { UpdateLibraryStatus(); };
	library_gallery_.WhenVisibleRange = [=](int first, int last) {
		PrepareLibraryImages(first, last);
		UpdateLibraryStatus();
	};
	library_gallery_.WhenAction = [=] {
		String id = library_projection_.GetCatalogId(library_gallery_.GetCursor());
		if(!id.IsEmpty())
			AddLibrarySelectionToCollection(id);
	};
	library_gallery_.WhenDragStart = [=] {
		String id = library_gallery_.GetDragCatalogId();
		Vector<String> ids = GetSelectedLibraryCatalogIds(id);
		Image image;
		int view_index = library_gallery_.GetDragViewIndex();
		if(view_index >= 0 && view_index < library_projection_.Model().GetCount())
			image = library_projection_.Model().Get(view_index).image;
		SetDragActive(true);
		ShowDragPreview(image, max(1, ids.GetCount()), GetMousePos());
	};
	library_gallery_.WhenDragMove = [=](Point screen) { MoveDragPreview(screen); };
	library_gallery_.WhenDragFinished = [=](SymbolPickerGestureResult result) {
		if(result.completed && collection_gallery_.GetScreenRect().Contains(result.release_screen))
			AddLibrarySelectionToCollection(result.catalog_id);
		SetDragActive(false);
	};

	collections_selector_.WhenAction = [=] {
		if(syncing_ || !model_ || !commands_)
			return;
		Value data = collections_selector_.GetSelectedData();
		if(IsNumber(data))
			commands_->Execute(MakeSymbolPickerSetActiveCollectionCommand((int)data), *model_);
	};
	new_collection_.WhenAction = [=] { CreateCollection(); };
	remove_collection_.WhenAction = [=] { RemoveActiveCollection(); };
	save_button_.WhenAction = [=] { file_actions_.SaveProject(false); };
	save_button_.WhenSelect = [=](int, const Value& data) {
		file_actions_.SaveProject(AsString(data) == "save_as");
	};
	load_button_.WhenAction = [=] { file_actions_.LoadProject(); };
	export_button_.WhenAction = [=] { file_actions_.Export(SymbolPickerExportScope::ActiveCollection); };
	export_button_.WhenSelect = [=](int, const Value& data) {
		file_actions_.Export(AsString(data) == "all" ? SymbolPickerExportScope::AllCollections
		                                          : SymbolPickerExportScope::ActiveCollection);
	};
	export_size_.WhenAction = [=] {
		if(syncing_ || !model_ || !commands_)
			return;
		Value data = export_size_.GetSelectedData();
		if(IsNumber(data))
			commands_->Execute(MakeSymbolPickerSetExportSizeCommand((int)data), *model_);
	};
	export_type_.WhenAction = [=] {
		if(syncing_ || !model_ || !commands_)
			return;
		Value data = export_type_.GetSelectedData();
		if(IsNumber(data))
			commands_->Execute(MakeSymbolPickerSetExportTypeCommand((SymbolPickerExportType)(int)data), *model_);
	};
	copy_button_.WhenAction = [=] { file_actions_.CopyCurrentExportToClipboard(); };
	delete_items_.WhenAction = [=] { RemoveSelectedCollectionItems(); };
	mark_used_.WhenAction = [=] { ToggleSelectedUsage(); };
	clear_collection_.WhenAction = [=] { ClearActiveCollection(); };
	collection_filter_.WhenAction = [=] { RefreshCollectionProjection(); };
	collection_filter_.WhenChange = [=] {
		if(syncing_)
			return;
		SetTimeCallback(70, [=] { RefreshCollectionProjection(); }, TIMEID_COLLECTION_FILTER);
	};
	collection_gallery_.WhenSelection = [=] {
		UpdateCollectionStatus();
		UpdateActionState();
	};
	collection_gallery_.WhenZoom = [=](double) { UpdateCollectionStatus(); };
	collection_gallery_.WhenVisibleRange = [=](int first, int last) {
		PrepareCollectionImages(first, last);
		UpdateCollectionStatus();
	};
	collection_gallery_.WhenDragStart = [=] {
		Image image;
		int view_index = collection_gallery_.GetDragViewIndex();
		if(view_index >= 0 && view_index < collection_projection_.Model().GetCount())
			image = collection_projection_.Model().Get(view_index).image;
		SetDragActive(true);
		ShowDragPreview(image, 1, GetMousePos());
	};
	collection_gallery_.WhenDragMove = [=](Point screen) { MoveDragPreview(screen); };
	collection_gallery_.WhenDragFinished = [=](SymbolPickerGestureResult result) {
		if(result.completed && collection_gallery_.GetScreenRect().Contains(result.release_screen))
			ReorderCollectionItem(result);
		SetDragActive(false);
	};
	library_style_.WhenSelectData = [=](const Value&) { library_style_.WhenAction(); };
	collections_selector_.WhenSelectData = [=](const Value&) { collections_selector_.WhenAction(); };
	export_size_.WhenSelectData = [=](const Value&) { export_size_.WhenAction(); };
	export_type_.WhenSelectData = [=](const Value&) { export_type_.WhenAction(); };
}

void SymbolPickerWorkspaceView::SetLibraryAssetKind(int kind)
{
 library_asset_kind_ = kind;
 seen_library_revision_ = -1;
 if(model_) {
  model_->SetCurrentCategory("All");
  model_->SetFilterText("");
 }
 RefreshFromModel();
}
void SymbolPickerWorkspaceView::SetModel(SymbolPickerModel* model)
{
	model_ = model;
	seen_library_revision_ = seen_collections_revision_ = seen_export_revision_ = seen_project_revision_ = -1;
	seen_active_collection_ = -2;
	file_actions_.SetContext(model_, catalog_, commands_);
	RefreshFromModel();
}

void SymbolPickerWorkspaceView::SetCatalog(SymbolPickerCatalog* catalog)
{
	catalog_ = catalog;
	++catalog_generation_;
	seen_catalog_generation_ = -1;
	image_cache_.Clear();
	image_style_ = -1;
	if(catalog_)
		image_cache_.SetMaxEntries(max(8192, catalog_->GetIcons().GetCount() + 1024));
	file_actions_.SetContext(model_, catalog_, commands_);
	RefreshFromModel();
}

void SymbolPickerWorkspaceView::SetCommands(SymbolPickerCommandStack* commands)
{
	commands_ = commands;
	file_actions_.SetContext(model_, catalog_, commands_);
	RefreshFromModel();
}

void SymbolPickerWorkspaceView::RefreshFromModel()
{
	if(suppress_refresh_) {
		refresh_pending_ = true;
		return;
	}
	if(!model_)
		return;

	EnsureImageGeneration();
	bool catalog_changed = seen_catalog_generation_ != catalog_generation_;
	bool library_changed = catalog_changed || seen_library_revision_ != model_->GetLibraryRevision();
	bool collections_changed = catalog_changed || seen_collections_revision_ != model_->GetCollectionsRevision();
	bool export_changed = seen_export_revision_ != model_->GetExportRevision();
	bool project_changed = seen_project_revision_ != model_->GetProjectRevision();

	syncing_ = true;
	if(library_changed)
		SyncLibraryControls();
	if(export_changed)
		SyncExportControls();
	syncing_ = false;

	if(library_changed) {
		RefreshCategoryButtons();
		RefreshLibraryProjection();
	}
	if(collections_changed) {
		bool switched = seen_active_collection_ != model_->GetActiveCollectionIndex();
		if(switched) { collection_filter_.SetTextUtf8(""); collection_asset_view_.SetDataSilently(-1); }
		RefreshCollectionSelector();
		RefreshCollectionProjection(!switched);
		seen_active_collection_ = model_->GetActiveCollectionIndex();
	}
	if(project_changed || collections_changed)
		SyncProjectUi();

	seen_catalog_generation_ = catalog_generation_;
	seen_library_revision_ = model_->GetLibraryRevision();
	seen_collections_revision_ = model_->GetCollectionsRevision();
	seen_export_revision_ = model_->GetExportRevision();
	seen_project_revision_ = model_->GetProjectRevision();

	UpdateLibraryStatus();
	UpdateCollectionStatus();
	UpdateActionState();
}

void SymbolPickerWorkspaceView::SyncLibraryControls()
{
	library_style_.SetDataSilently((int)model_->GetIconStyle());
	library_optical_.SetDataSilently(model_->GetOpticalSize());
	library_filled_.SetChecked(model_->GetFilled());
	if(library_filter_.GetTextUtf8() != model_->GetFilterText())
		library_filter_.SetTextUtf8(model_->GetFilterText());
	library_tint_ <<= model_->GetTintColor();
}

void SymbolPickerWorkspaceView::SyncExportControls()
{
	export_size_.SetDataSilently(model_->GetExportSize());
	export_type_.SetDataSilently((int)model_->GetExportType());
}

void SymbolPickerWorkspaceView::SyncProjectUi()
{
	String name = TrimBoth(model_->GetProjectName());
	if(name.IsEmpty())
		name = "Untitled project";
	int dirty = 0;
	for(const auto& collection : model_->GetCollections())
		dirty += collection.dirty ? 1 : 0;
	project_label_.SetText(dirty ? Format("%s · %d unsaved collection%s", name, dirty, dirty == 1 ? "" : "s") : name);
	Title("Symbol Picker — " + name);
}

void SymbolPickerWorkspaceView::RefreshCategoryButtons()
{
	categories_flow_.ClearItems();
	category_buttons_.Clear();

	String selected = model_ ? model_->GetCurrentCategory() : String("All");
	categories_title_.SetText(library_asset_kind_ == 1 ? "Image categories" : "Icon categories");
	symbols_view_.SetChecked(library_asset_kind_ == 0); images_view_.SetChecked(library_asset_kind_ == 1);
	library_style_.Enable(library_asset_kind_ == 0);
	library_optical_.Enable(library_asset_kind_ == 0);
	library_filled_.Enable(library_asset_kind_ == 0);
	String filter = categories_filter_.GetTextUtf8();
	Vector<SymbolPickerCategory> categories;
	int total = 0;
	if(catalog_) {
		SymbolPickerIconStyle style = model_ ? model_->GetIconStyle() : SymbolPickerIconStyle::Outlined;
		categories = catalog_->GetCategories(style, library_asset_kind_, model_ ? model_->GetOpticalSize() : 20, model_ && model_->GetFilled());
		for(const auto& category : categories)
			total += category.icon_count;
	}

	auto AddCategory = [&](const String& id, const String& text, int palette_index) {
		UiButton& button = category_buttons_.Add(new UiButton());
		auto style = UiTheme::ResolveButton();
		const auto t = UiTheme::GetOliveTokens(UiTheme::GetMode());
		Color tint = CategoryTint(palette_index, categories.GetCount() + 1);
		Color face = tint;
		style.skin = StyledSkin();
		style.metrics.face_enabled = true;
		style.metrics.radius = DPI(8);
		style.palette.face[ST_NORMAL] = UiFill::Solid(face);
		style.metrics.frame_width = 1;
		style.metrics.content_margin = Rect(DPI(4), DPI(3), DPI(4), DPI(3));
		style.palette.face[ST_HOT] = UiFill::Solid(Blend(face, t.accent, 24));
		style.palette.face[ST_PRESSED] = UiFill::Solid(t.active);
		style.palette.ink[ST_NORMAL] = style.palette.ink[ST_HOT] = t.text;
		style.palette.ink[ST_PRESSED] = t.accent;
		style.palette.frame[ST_NORMAL] = style.palette.frame[ST_HOT] = t.line_soft;
		style.palette.frame[ST_PRESSED] = t.accent;
		button.SetCustomStyle(style);
		button.SetText(text).SetCheckable(true).SetChecked(selected == id).SetContentInset(DPI(4));
		categories_flow_.Add(button).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
		Ptr<UiButton> category_button = &button;
		button.WhenAction = [=] {
			if(category_button) category_button->SetChecked(true);
			PostCallback([=] {
				if(model_ && commands_)
					commands_->Execute(MakeSymbolPickerSetCategoryCommand(id), *model_);
			});
		};
	};

	AddCategory("All", catalog_ ? Format("All (%d)", total) : String("All"), 0);
	int palette_index = 0;
	for(const auto& category : categories) {
		++palette_index;
		String text = Format("%s (%d)", category.display_name, category.icon_count);
		if(MatchText(text, filter))
			AddCategory(category.id, text, palette_index);
	}
}

void SymbolPickerWorkspaceView::RefreshLibraryProjection(bool preserve_selection)
{
	Value selection = preserve_selection ? library_gallery_.GetData() : Value();
	if(!catalog_ || !model_) {
		library_projection_.Clear();
		library_gallery_.ClearSelection();
		UpdateLibraryStatus();
		return;
	}

#ifdef _DEBUG
	int64 started = msecs();
#endif
	library_projection_.Rebuild(*catalog_, model_->GetCurrentCategory(), model_->GetFilterText(), model_->GetIconStyle(), library_asset_kind_, library_include_metadata_.IsChecked(), model_->GetOpticalSize(), model_->GetFilled());
	String category = model_->GetCurrentCategory();
	auto categories = catalog_->GetCategories(model_->GetIconStyle(), library_asset_kind_, model_->GetOpticalSize(), model_->GetFilled());
	for(int i = 0; i < categories.GetCount(); ++i)
		if(categories[i].id == category) {
			category = categories[i].display_name;
			break;
		}
	auto heading = HeadingStyle();
	library_title_.SetCustomStyle(heading).SetText(String(library_asset_kind_ == 1 ? "Images · " : "Library · ") + category + Format(" · %d", library_projection_.GetCount()));

	if(preserve_selection)
		library_gallery_.SetData(selection);
	else
		library_gallery_.ClearSelection();
	UiVisibleRange range = library_gallery_.GetVisibleRange(true);
	if(!range.IsEmpty())
		PrepareLibraryImages(range.first, range.last);
#ifdef _DEBUG
	RLOG(Format("SymbolPicker library projection items=%d renderers=%d cache=%d hit=%d miss=%d elapsed_ms=%d",
		library_projection_.GetCount(), library_gallery_.GetLiveItemRenderCount(), image_cache_.GetCount(),
		image_cache_.GetHitCount(), image_cache_.GetMissCount(), (int)(msecs() - started)));
#endif
	UpdateLibraryStatus();
}

void SymbolPickerWorkspaceView::RefreshCollectionSelector()
{
	collections_selector_.UseInternalModel().Clear();
	if(!model_)
		return;
	const Vector<SymbolPickerCollection>& collections = model_->GetCollections();
	for(int i = 0; i < collections.GetCount(); ++i) {
		const SymbolPickerCollection& collection = collections[i];
		collections_selector_.Add(collection.name + (collection.dirty ? " *" : String()), i);
	}
	if(model_->GetActiveCollectionIndex() >= 0)
		collections_selector_.SetDataSilently(model_->GetActiveCollectionIndex());
	else
		collections_selector_.ClearSelection();
}

void SymbolPickerWorkspaceView::RefreshCollectionProjection(bool preserve_selection)
{
	Value selection = preserve_selection ? collection_gallery_.GetData() : Value();
	const SymbolPickerCollection* active = model_ ? model_->GetActiveCollection() : nullptr;
	if(!catalog_ || !active) {
		collection_projection_.Clear();
		collection_gallery_.ClearSelection();
		UpdateCollectionStatus();
		UpdateActionState();
		return;
	}

	collection_projection_.Rebuild(*active, *catalog_, collection_filter_.GetTextUtf8(), (int)collection_asset_view_.GetData());
	if(preserve_selection)
		collection_gallery_.SetData(selection);
	else
		collection_gallery_.ClearSelection();
	UiVisibleRange range = collection_gallery_.GetVisibleRange(true);
	if(!range.IsEmpty())
		PrepareCollectionImages(range.first, range.last);
	UpdateCollectionStatus();
	UpdateActionState();
}

void SymbolPickerWorkspaceView::EnsureImageGeneration()
{
	if(!model_)
		return;
	int style = (int)model_->GetIconStyle();
	Color tint = GetLibraryPreviewTint();
	bool dark = UiTheme::GetMode() == UiThemeMode::Dark;
	if(style != image_style_ || tint != image_tint_ || dark != image_dark_) {
		image_cache_.Clear();
		image_style_ = style;
		image_tint_ = tint;
		image_dark_ = dark;
	}
}

Color SymbolPickerWorkspaceView::GetLibraryPreviewTint() const
{
	Color tint = model_ ? model_->GetTintColor() : Color(Null);
	return IsNull(tint) ? UiTheme::GetOliveTokens(UiTheme::GetMode()).text : tint;
}

Color SymbolPickerWorkspaceView::GetCollectionPreviewTint(const SymbolPickerIconRef& ref) const
{
	return IsNull(ref.tint) ? UiTheme::GetOliveTokens(UiTheme::GetMode()).text : ref.tint;
}

void SymbolPickerWorkspaceView::PrepareLibraryImages(int first, int last)
{
	if(preparing_library_images_ || !catalog_ || !model_ || library_projection_.Model().IsEmpty())
		return;
	first = max(0, first);
	last = min(last, library_projection_.Model().GetCount() - 1);
	if(last < first)
		return;

	preparing_library_images_ = true;
	int changed_first = -1, changed_last = -1;
	for(int i = first; i <= last; ++i) {
		const UiModelItem& current = library_projection_.Model().Get(i);
		if(!current.image.IsEmpty())
			continue;
		const SymbolPickerIconEntry* entry = library_projection_.GetEntry(i);
		if(!entry)
			continue;
		// Theme contrast applies to built-in UI symbols. Imported artwork keeps
		// its colours unless the user explicitly chooses a tint.
		Color tint = entry->external_file.IsEmpty() ? GetLibraryPreviewTint() : model_->GetTintColor();
		Image image = image_cache_.GetImage(*entry, DPI(kPreviewPx), tint);
		if(image.IsEmpty())
			continue;
		library_projection_.Model().Get(i).image = image;
		if(changed_first < 0) changed_first = i;
		changed_last = i;
	}
	if(changed_first >= 0)
		library_projection_.Model().Touch(changed_first, changed_last - changed_first + 1);
	preparing_library_images_ = false;
}

void SymbolPickerWorkspaceView::PrepareCollectionImages(int first, int last)
{
	if(preparing_collection_images_ || !catalog_ || !model_ || collection_projection_.Model().IsEmpty())
		return;
	const SymbolPickerCollection* active = model_->GetActiveCollection();
	if(!active)
		return;
	first = max(0, first);
	last = min(last, collection_projection_.Model().GetCount() - 1);
	if(last < first)
		return;

	preparing_collection_images_ = true;
	int changed_first = -1, changed_last = -1;
	for(int i = first; i <= last; ++i) {
		const UiModelItem& current = collection_projection_.Model().Get(i);
		if(!current.image.IsEmpty())
			continue;
		int item_index = collection_projection_.GetItemIndex(i);
		if(item_index < 0 || item_index >= active->items.GetCount())
			continue;
		const SymbolPickerIconRef& ref = active->items[item_index];
		const SymbolPickerIconEntry* entry = catalog_->FindByCatalogId(ref.catalog_id);
		if(!entry)
			continue;
		Color tint = entry->external_file.IsEmpty() ? GetCollectionPreviewTint(ref) : ref.tint;
		Image image = image_cache_.GetImage(*entry, DPI(kPreviewPx), tint);
		if(image.IsEmpty())
			continue;
		collection_projection_.Model().Get(i).image = image;
		if(changed_first < 0) changed_first = i;
		changed_last = i;
	}
	if(changed_first >= 0)
		collection_projection_.Model().Touch(changed_first, changed_last - changed_first + 1);
	preparing_collection_images_ = false;
}

void SymbolPickerWorkspaceView::UpdateLibraryStatus()
{
	String text;
	if(library_gallery_.GetSelectionCount())
		text << Format("%d selected", library_gallery_.GetSelectionCount());
	String zoom = Format("%.0f%%", library_gallery_.GetZoom() * 100.0);
	if(library_zoom_.GetTextUtf8() != zoom) library_zoom_.SetTextUtf8(zoom);
	library_zoom_in_.Enable(library_gallery_.GetZoom() < library_gallery_.GetMaxZoom());
	library_zoom_out_.Enable(library_gallery_.GetZoom() > library_gallery_.GetMinZoom());
#ifdef _DEBUG
	text << Format(" · renderers %d · cache %d/%d", library_gallery_.GetLiveItemRenderCount(),
		image_cache_.GetCount(), image_cache_.GetMaxEntries());
#endif
	library_status_.SetText(text);
}

void SymbolPickerWorkspaceView::UpdateCollectionStatus()
{
	const SymbolPickerCollection* active = model_ ? model_->GetActiveCollection() : nullptr;
	if(!active) {
		collections_status_.ClearSpans().SetText("No active collection");
		return;
	}
	String text = Format("%d items", active->items.GetCount());
	if(collection_projection_.GetCount() != active->items.GetCount())
		text << Format(" · %d shown", collection_projection_.GetCount());
	int used = 0;
	for(const auto& item : active->items) if(item.marked_used) ++used;
	collections_status_.ClearSpans().AddTextSpan(text);
	if(used) {
		ImageBuffer marker(DPI(6), DPI(6));
		Fill(~marker, UiTheme::GetOliveTokens(UiTheme::GetMode()).accent, marker.GetLength());
		collections_status_.AddTextSpan("    ").AddIconSpan(Image(marker))
			.AddTextSpan(Format(" %d marked in use", used));
	}
	text.Clear();
	if(collection_gallery_.GetSelectionCount())
		text << Format(" · %d selected", collection_gallery_.GetSelectionCount());
	if(!text.IsEmpty()) collections_status_.AddTextSpan(text);
}

void SymbolPickerWorkspaceView::UpdateActionState()
{
	const SymbolPickerCollection* active = model_ ? model_->GetActiveCollection() : nullptr;
	bool has_active = active != nullptr;
	bool has_items = has_active && !active->items.IsEmpty();
	remove_collection_.Enable(has_active);
	delete_items_.Enable(has_active && collection_gallery_.GetSelectionCount() > 0);
	mark_used_.Enable(has_active && collection_gallery_.GetSelectionCount() > 0);
	bool all_used = has_active && collection_gallery_.GetSelectionCount() > 0;
	if(active) for(int i : GetSelectedCollectionItemIndexes()) all_used = all_used && active->items[i].marked_used;
	mark_used_.SetText(all_used ? "Unmark used" : "Mark used");
	String usage_tip = "A blue dot marks an icon as used. This is a warning, not an automatic proof of usage.";
	if(active) for(int i : GetSelectedCollectionItemIndexes()) {
		if(!active->items[i].usage_note.IsEmpty()) usage_tip << "\n" << active->items[i].alias << ": " << active->items[i].usage_note;
	}
	mark_used_.Tip(usage_tip);

	clear_collection_.Enable(has_items);
	export_button_.Enable(has_items);
	copy_button_.Enable(has_items);
}

void SymbolPickerWorkspaceView::UpdateThemeButton()
{
	theme_button_.SetIcon(UiTheme::GetMode() == UiThemeMode::Dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());
}

void SymbolPickerWorkspaceView::ApplyLibraryFilter()
{
	if(syncing_ || !model_ || !commands_)
		return;
	commands_->Execute(MakeSymbolPickerSetFilterCommand(library_filter_.GetTextUtf8()), *model_);
}

bool SymbolPickerWorkspaceView::RunSearchSmokeTests(String& error)
{
	// Use the actual selection event emitted by UiDropdown.
	SymbolPickerProject original = model_->ExportProject();
	SymbolPickerProject choices;
	for(int c = 0; c < 2; ++c) {
		auto& collection = choices.collections.Add(); collection.name = c ? "Second app" : "First app";
		for(int i = 0; i <= c; ++i) { auto& item = collection.items.Add(); item.catalog_id = catalog_->GetIcons()[i + c * 2].catalog_id; item.alias = "TEST_" + AsString(i); }
	}
	choices.active_collection_index = 0; model_->LoadProject(choices);
	collections_selector_.Select(1);
	if(model_->GetActiveCollectionIndex() != 1 || collection_gallery_.Model().GetCount() != 2 || collection_projection_.GetCatalogId(0) != choices.collections[1].items[0].catalog_id) { error = "Selecting a different collection did not replace its gallery."; return false; }
	library_style_.SelectByData((int)SymbolPickerIconStyle::Sharp);
	if(model_->GetIconStyle() != SymbolPickerIconStyle::Sharp) { error = "Selecting a symbol style did not update the library."; return false; }
	export_size_.SelectByData(64); export_type_.SelectByData((int)SymbolPickerExportType::PngFiles);
	if(model_->GetExportSize() != 64 || model_->GetExportType() != SymbolPickerExportType::PngFiles) { error = "Export dropdown selection did not update its setting."; return false; }
	SetLibraryAssetKind(1);
	if(library_gallery_.Model().GetCount() != catalog_->Filter("All", "", model_->GetIconStyle(), 1).GetCount() || library_style_.IsEnabled()) { error = "The Images view mixed SVG icons or retained the symbol-style selector."; return false; }
	SetLibraryAssetKind(0); model_->LoadProject(original);
	for(const char* query : { "camera", "no-symbol-matches-this-query", "" }) {
		library_filter_.SetTextUtf8(query);
		library_filter_.SetCursor(library_filter_.GetTextUtf8().GetCount());
		int64 caret = library_filter_.GetCursor();
		library_filter_.WhenChange();
		if(!ExistsTimeCallback(TIMEID_LIBRARY_FILTER)) { error = "Library debounce was not scheduled."; return false; }
		KillTimeCallback(TIMEID_LIBRARY_FILTER);
		ApplyLibraryFilter();
		if(library_filter_.GetCursor() != caret) { error = "Library filter refresh moved the caret."; return false; }
		if(library_gallery_.Model().GetCount() != catalog_->Filter(model_->GetCurrentCategory(), query, model_->GetIconStyle(), library_asset_kind_, library_include_metadata_.IsChecked(), model_->GetOpticalSize(), model_->GetFilled()).GetCount()) {
			error = "Library filter count differs from catalogue."; return false;
		}
		collection_filter_.SetTextUtf8(query);
		collection_filter_.WhenChange();
		if(!ExistsTimeCallback(TIMEID_COLLECTION_FILTER)) { error = "Collection debounce was not scheduled."; return false; }
		KillTimeCallback(TIMEID_COLLECTION_FILTER);
		RefreshCollectionProjection();
	}
	double original_zoom = library_gallery_.GetZoom();
	library_zoom_.SetTextUtf8("125%");
	library_zoom_.WhenAction();
	if(abs(library_gallery_.GetZoom() - 1.25) > 0.001) { error = "Typed zoom did not apply."; return false; }
	library_zoom_out_.WhenAction();
	library_zoom_in_.WhenAction();
	if(abs(library_gallery_.GetZoom() - 1.25) > 0.001) { error = "Zoom buttons did not round-trip."; return false; }
	library_zoom_.SetTextUtf8("invalid");
	library_zoom_.WhenAction();
	if(library_zoom_.GetTextUtf8() != "125%") { error = "Invalid zoom was not restored."; return false; }
	library_gallery_.SetZoom(original_zoom);
	// Preview background stays independent of exported foreground and survives theme changes.
	Color preview_test_color(65, 72, 58);
	preview_background_ <<= preview_test_color;
	preview_background_.WhenAction();
	if((Color)~preview_background_ != preview_test_color ||
	   model_->GetTintColor() != original.default_tint) {
		error = "Preview background changed the exported foreground tint."; return false;
	}
	// Exercise the same theme transition as the toolbar, including custom surfaces.
	for(int transition = 0; transition < 4; ++transition) {
		theme_button_.WhenAction();
		const auto surface = UiTheme::ResolvePanel();
		if(UiTheme::GetPreset() != UiThemePreset::Olive || (Color)~preview_background_ != preview_test_color) {
			error = "Theme transition lost Olive or the chosen preview background."; return false;
		}
		for(UiPanel* panel : { &categories_panel_, &library_panel_, &collections_panel_ })
			if(panel->GetStyle().palette.face[ST_NORMAL].color != surface.palette.face[ST_NORMAL].color) {
				error = "Panel retained a surface from the previous theme."; return false;
			}
		for(SymbolPickerDragGallery* gallery : { &library_gallery_, &collection_gallery_ })
			if(gallery->GetStyle().palette.face[ST_NORMAL].color != surface.palette.face[ST_NORMAL].color) {
				error = "Gallery retained a surface from the previous theme."; return false;
			}
		const auto edit = UiTheme::ResolveEdit();
		if(library_filter_.GetStyle().palette.face[ST_NORMAL].color != edit.palette.face[ST_NORMAL].color) {
			error = "Search entry did not follow the current theme."; return false;
		}
	}
	preview_background_ <<= Color(Null);
	preview_background_.WhenAction();
	error.Clear();
	return true;
}

void SymbolPickerWorkspaceView::CreateCollection()
{
	if(!model_ || !commands_)
		return;
	int new_index = model_->GetCollections().GetCount();
	String name = "App icons";
	if(!PromptCollectionName(name)) return;
	BeginCommandBatch();
	commands_->BeginGroup("Create collection");
	bool created = commands_->Execute(MakeSymbolPickerCreateCollectionCommand(name), *model_);
	if(created)
		commands_->Execute(MakeSymbolPickerSetActiveCollectionCommand(new_index), *model_);
	commands_->EndGroup();
	EndCommandBatch();
}

void SymbolPickerWorkspaceView::RemoveActiveCollection()
{
	if(!model_ || !commands_ || model_->GetActiveCollectionIndex() < 0)
		return;
	if(!ConfirmUsedRemoval(*model_->GetActiveCollection(), Vector<int>())) return;
	collection_gallery_.ClearSelection();
	BeginCommandBatch();
	commands_->Execute(MakeSymbolPickerRemoveCollectionCommand(model_->GetActiveCollectionIndex()), *model_);
	EndCommandBatch();
}

void SymbolPickerWorkspaceView::ClearActiveCollection()
{
	if(!model_ || !commands_ || model_->GetActiveCollectionIndex() < 0)
		return;
	if(!ConfirmUsedRemoval(*model_->GetActiveCollection(), Vector<int>())) return;
	collection_gallery_.ClearSelection();
	BeginCommandBatch();
	commands_->Execute(MakeSymbolPickerClearCollectionCommand(model_->GetActiveCollectionIndex()), *model_);
	EndCommandBatch();
}

Vector<int> SymbolPickerWorkspaceView::GetSelectedCollectionItemIndexes() const
{
	Index<int> unique;
	for(int view_index : collection_gallery_.GetSelection()) {
		int item_index = collection_projection_.GetItemIndex(view_index);
		if(item_index >= 0)
			unique.FindAdd(item_index);
	}
	Vector<int> out;
	for(int i = 0; i < unique.GetCount(); ++i)
		out.Add(unique[i]);
	return out;
}

bool SymbolPickerWorkspaceView::ConfirmUsedRemoval(const SymbolPickerCollection& collection, const Vector<int>& indexes)
{
	int used = 0;
	if(indexes.IsEmpty()) {
		for(const auto& item : collection.items) if(item.marked_used) ++used;
	}
	else for(int i : indexes) if(collection.items[i].marked_used) ++used;
	return !used || PromptYesNo(Format("%d icon(s) are marked as used in code. Removing them and re-exporting may break callers. Continue?", used));
}

void SymbolPickerWorkspaceView::ToggleSelectedUsage()
{
	const auto* active = model_ ? model_->GetActiveCollection() : nullptr;
	if(!active || !commands_) return;
	auto indexes = GetSelectedCollectionItemIndexes();
	if(indexes.IsEmpty()) return;
	bool all_used = true;
	for(int i : indexes) all_used = all_used && active->items[i].marked_used;
	BeginCommandBatch();
	commands_->BeginGroup("Mark icon usage");
	for(int i : indexes)
		commands_->Execute(MakeSymbolPickerSetIconUsedCommand(model_->GetActiveCollectionIndex(), i, !all_used), *model_);
	commands_->EndGroup();
	EndCommandBatch();
}

void SymbolPickerWorkspaceView::RemoveSelectedCollectionItems()
{
	if(!model_ || !commands_ || model_->GetActiveCollectionIndex() < 0)
		return;
	Vector<int> indexes = GetSelectedCollectionItemIndexes();
	if(indexes.IsEmpty())
		return;
	if(!ConfirmUsedRemoval(*model_->GetActiveCollection(), indexes)) return;
	Sort(indexes, StdGreater<int>());
	collection_gallery_.ClearSelection();
	BeginCommandBatch();
	if(indexes.GetCount() > 1)
		commands_->BeginGroup("Remove collection icons");
	for(int item_index : indexes)
		commands_->Execute(MakeSymbolPickerRemoveIconFromCollectionCommand(model_->GetActiveCollectionIndex(), item_index), *model_);
	if(indexes.GetCount() > 1)
		commands_->EndGroup();
	EndCommandBatch();
}

Vector<String> SymbolPickerWorkspaceView::GetSelectedLibraryCatalogIds(const String& primary_catalog_id) const
{
	Vector<String> out;
	Vector<int> selected = library_gallery_.GetSelection();
	bool primary_selected = false;
	for(int view_index : selected)
		if(library_projection_.GetCatalogId(view_index) == primary_catalog_id) {
			primary_selected = true;
			break;
		}
	if(primary_selected) {
		for(int view_index : selected) {
			String id = library_projection_.GetCatalogId(view_index);
			if(!id.IsEmpty())
				out.Add(id);
		}
	}
	else if(!primary_catalog_id.IsEmpty())
		out.Add(primary_catalog_id);
	return out;
}

String SymbolPickerWorkspaceView::MakeCollectionAlias(const SymbolPickerIconEntry& entry) const
{
	if(entry.image_factory)
		return entry.source_symbol;
	return "ICON_" + SafeAliasPart(entry.category) + "_" + SafeAliasPart(entry.display_name) + "_" + SafeAliasPart(IconStyleText(entry.style));
}

void SymbolPickerWorkspaceView::AddLibrarySelectionToCollection(const String& primary_catalog_id)
{
	if(!model_ || !catalog_ || !commands_)
		return;
	if(model_->GetActiveCollectionIndex() < 0)
		CreateCollection();
	if(model_->GetActiveCollectionIndex() < 0)
		return;

	Vector<String> ids = GetSelectedLibraryCatalogIds(primary_catalog_id);
	if(ids.IsEmpty())
		return;

	Index<String> present;
	const SymbolPickerCollection* active = model_->GetActiveCollection();
	if(active)
		for(const auto& item : active->items)
			if(!item.catalog_id.IsEmpty())
				present.FindAdd(item.catalog_id);

	BeginCommandBatch();
	if(ids.GetCount() > 1)
		commands_->BeginGroup("Add symbols to collection");
	int added = 0;
	int duplicates = 0;
	for(const String& id : ids) {
		if(present.Find(id) >= 0) {
			++duplicates;
			continue;
		}
		const SymbolPickerIconEntry* entry = catalog_->FindByCatalogId(id);
		if(!entry || !entry->available)
			continue;
		SymbolPickerIconRef ref;
		ref.catalog_id = entry->catalog_id;
		ref.source_id = entry->source_id;
		ref.alias = MakeCollectionAlias(*entry);
		ref.size = model_->GetExportSize();
		ref.tint = model_->GetTintColor();
		if(commands_->Execute(MakeSymbolPickerAddIconToCollectionCommand(model_->GetActiveCollectionIndex(), ref), *model_)) {
			present.FindAdd(id);
			++added;
		}
	}
	if(ids.GetCount() > 1)
		commands_->EndGroup();
	EndCommandBatch();

	if(duplicates)
		library_status_.SetText(added ? Format("%d added · %d duplicate%s skipped", added, duplicates, duplicates == 1 ? "" : "s")
		                              : Format("%d duplicate%s skipped", duplicates, duplicates == 1 ? "" : "s"));
}

int SymbolPickerWorkspaceView::GetCollectionDropInsertIndex(Point screen) const
{
	const SymbolPickerCollection* active = model_ ? model_->GetActiveCollection() : nullptr;
	if(!active)
		return 0;
	Point local = screen - collection_gallery_.GetScreenRect().TopLeft();
	UiVisibleRange range = collection_gallery_.GetVisibleRange(false);
	if(!range.IsEmpty()) {
		for(int view_index = range.first; view_index <= range.last; ++view_index) {
			Rect r = collection_gallery_.GetItemRect(view_index);
			if(!r.Contains(local))
				continue;
			int underlying = collection_projection_.GetItemIndex(view_index);
			if(underlying < 0)
				break;
			bool after = collection_gallery_.GetColumnCount() > 1
				? local.x >= r.CenterPoint().x
				: local.y >= r.CenterPoint().y;
			return underlying + (after ? 1 : 0);
		}
	}
	return active->items.GetCount();
}

void SymbolPickerWorkspaceView::ReorderCollectionItem(const SymbolPickerGestureResult& result)
{
	if(!model_ || !commands_ || model_->GetActiveCollectionIndex() < 0 || result.collection_index < 0)
		return;
	int to_index = GetCollectionDropInsertIndex(result.release_screen);
	collection_gallery_.ClearSelection();
	BeginCommandBatch();
	commands_->Execute(MakeSymbolPickerMoveCollectionIconCommand(model_->GetActiveCollectionIndex(), result.collection_index, to_index), *model_);
	EndCommandBatch();
}

void SymbolPickerWorkspaceView::BeginCommandBatch()
{
	suppress_refresh_ = true;
	refresh_pending_ = false;
}

void SymbolPickerWorkspaceView::EndCommandBatch()
{
	suppress_refresh_ = false;
	bool pending = refresh_pending_;
	refresh_pending_ = false;
	if(pending)
		RefreshFromModel();
}

void SymbolPickerWorkspaceView::SetDragActive(bool active)
{
	drag_active_ = active;
	if(!active)
		HideDragPreview();
}

void SymbolPickerWorkspaceView::ShowDragPreview(const Image& image, int count, Point screen)
{
	drag_preview_.SetPreview(image, count);
	MoveDragPreview(screen);
	drag_preview_.Show();
}

void SymbolPickerWorkspaceView::MoveDragPreview(Point screen)
{
	Point local = screen - GetScreenRect().TopLeft() + Point(DPI(14), DPI(14));
	Size size = drag_preview_.GetMinSize();
	int x = min(max(0, local.x), max(0, GetSize().cx - size.cx));
	int y = min(max(0, local.y), max(0, GetSize().cy - size.cy));
	drag_preview_.SetRect(x, y, size.cx, size.cy);
}

void SymbolPickerWorkspaceView::HideDragPreview()
{
	drag_preview_.Hide();
}

bool SymbolPickerWorkspaceView::Key(dword key, int count)
{
	if(key == K_DELETE) {
		RemoveSelectedCollectionItems();
		return true;
	}
	if(key == K_CTRL_Z && model_ && commands_) {
		BeginCommandBatch();
		bool ok = commands_->Undo(*model_);
		EndCommandBatch();
		return ok;
	}
	if(key == K_CTRL_Y && model_ && commands_) {
		BeginCommandBatch();
		bool ok = commands_->Redo(*model_);
		EndCommandBatch();
		return ok;
	}
	return TopWindow::Key(key, count);
}

} // namespace Upp
