#include "SymbolPickerWorkspaceView.h"

namespace Upp {

namespace {

static constexpr int kPreviewPx = 32;
static constexpr int kLibraryFilterCallbackId = 8201;
static constexpr int kCollectionFilterCallbackId = 8202;

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
	Color frame = Color(59, 130, 246);
	w.DrawRect(r, SColorPaper());
	w.DrawRect(r.left, r.top, r.GetWidth(), 1, frame);
	w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, frame);
	w.DrawRect(r.left, r.top, 1, r.GetHeight(), frame);
	w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), frame);
	if(!image_.IsEmpty())
		w.DrawImage(DPI(8), DPI(8), DPI(32), DPI(32), image_);
	if(count_ > 1)
		w.DrawText(DPI(46), DPI(17), Format("%d items", count_), SansSerifZ(9).Bold(), SColorText());
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
	root_.Add(library_panel_).Expand(3).MinMain(DPI(230)).AlignSelf(UiBoxLayout::Align::Stretch);
	root_.Add(collections_panel_).Expand(2).MinMain(DPI(230)).AlignSelf(UiBoxLayout::Align::Stretch);

	Add(drag_preview_);
}

void SymbolPickerWorkspaceView::BuildHeader()
{
	header_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	app_title_.SetCustomStyle(HeadingStyle()).SetText("Symbol Picker");
	project_label_.SetCustomStyle(StatusStyle()).SetText("Model-driven icon workspace");
	theme_button_.SetText("Dark");
	help_button_.SetText("Help");
	exit_button_.SetText("Exit");

	header_.Add(app_title_).Fit().MinMain(DPI(120));
	header_.Add(project_label_).Fit().MinMain(DPI(220));
	header_.AddSpacer(1).Expand(1).MinMain(DPI(10));
	header_.Add(theme_button_).Fixed(DPI(64));
	header_.Add(help_button_).Fixed(DPI(64));
	header_.Add(exit_button_).Fixed(DPI(64));
}

void SymbolPickerWorkspaceView::BuildCategories()
{
	categories_panel_.Add(categories_box_.SizePos());
	categories_box_.SetDirection(UiDirection::V).SetGap(DPI(5)).SetInset(DPI(7));
	categories_header_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	categories_title_.SetCustomStyle(HeadingStyle()).SetText("Categories");
	categories_filter_.SetPlaceholder("Filter categories");

	categories_header_.Add(categories_title_).Fit().MinMain(DPI(120));
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
	library_header_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	library_actions_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0)
	                .SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true).SetAlignItems(UiCrossAlign::Center);

	library_title_.SetCustomStyle(HeadingStyle()).SetText("Library");
	library_status_.SetCustomStyle(StatusStyle()).SetText("No catalog loaded");
	library_header_.Add(library_title_).Fit().MinMain(DPI(90));
	library_header_.Add(library_status_).Expand(1).MinMain(DPI(220));

	library_style_.UseInternalModel().Clear()
		.Add("Outlined", (int)SymbolPickerIconStyle::Outlined)
		.Add("Rounded", (int)SymbolPickerIconStyle::Rounded)
		.Add("Sharp", (int)SymbolPickerIconStyle::Sharp);
	library_style_.SetSizeMin(DPI(118), 0);
	tint_label_.SetText("Tint");
	library_tint_ <<= Black();
	library_filter_.SetPlaceholder("Search symbols");

	library_actions_.Add(library_style_).Fit().MinMain(DPI(118));
	library_actions_.Add(tint_label_).Fit();
	library_actions_.Add(library_tint_).Fixed(DPI(42)).MinCross(DPI(24));
	library_actions_.AddSpacer(1).Expand(1).MinMain(DPI(8));
	library_actions_.Add(library_filter_).Expand(1).MinMain(DPI(220));

	library_gallery_.SetDragSource(SymbolPickerGestureSourceKind::Library)
	                .SetModel(library_projection_.Model())
	                .SetSelectionMode(UIGALLERYSEL_MULTI)
	                .SetItemSize(Size(DPI(92), DPI(92)))
	                .SetGap(DPI(6))
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
	collections_header_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
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
	collections_actions_.Add(clear_collection_).Fixed(DPI(64));
	collections_actions_.Add(collection_filter_).Expand(1).MinMain(DPI(170));

	collection_gallery_.SetDragSource(SymbolPickerGestureSourceKind::Collection)
	                   .SetModel(collection_projection_.Model())
	                   .SetSelectionMode(UIGALLERYSEL_MULTI)
	                   .SetItemSize(Size(DPI(176), DPI(82)))
	                   .SetGap(DPI(6))
	                   .SetInset(DPI(6))
	                   .SetOverscanRows(2)
	                   .SetZoomRange(0.72, 1.55, 1.12);

	collections_box_.Add(collections_header_).Fit();
	collections_box_.Add(collections_actions_).Fit();
	collections_box_.Add(collection_gallery_).Expand(1);
}

void SymbolPickerWorkspaceView::ConfigureGalleryRenderers()
{
	UiItemRenderImage library_render;
	UiItemRenderStyle library_style = library_render.GetStyle();
	library_style.image_extent = DPI(36);
	library_style.title_font = SansSerifZ(9);
	library_style.show_description = false;
	library_style.show_right_text = false;
	library_style.show_metadata = false;
	library_style.content_gap = DPI(4);
	library_render.SetCustomStyle(library_style);
	library_gallery_.SetItemRender(library_render);

	UiItemRenderImage collection_render;
	UiItemRenderStyle collection_style = collection_render.GetStyle();
	collection_style.image_extent = DPI(32);
	collection_style.title_font = SansSerifZ(9);
	collection_style.right_font = SansSerifZ(8);
	collection_style.show_description = false;
	collection_style.show_right_text = true;
	collection_style.show_metadata = true;
	collection_style.content_gap = DPI(5);
	collection_render.SetCustomStyle(collection_style);
	collection_gallery_.SetItemRender(collection_render);
}

void SymbolPickerWorkspaceView::WireActions()
{
	theme_button_.WhenAction = [=] {
		bool dark = UiTheme::GetMode() != UiThemeMode::Dark;
		UiTheme::Set(dark ? UiThemeMode::Dark : UiThemeMode::Light);
		Ctrl::SwapDarkLight();
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
	};
	help_button_.WhenAction = [=] {
		PromptOK("[*+130 Symbol Picker]&"
		         "Browse the full model-backed symbol catalogue without a per-icon control cost.&"
		         "Use search, categories and style to narrow the library; Ctrl/Shift or marquee selects multiple symbols.&"
		         "Drag selected symbols into the active collection, drag collection items to reorder, and use Ctrl+Z / Ctrl+Y for undo and redo.&"
		         "Rendered previews are prepared lazily for the visible Gallery range and reused from the image cache.");
	};
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
		   catalog_->Filter(model_->GetCurrentCategory(), String(), style).IsEmpty())
			commands_->Execute(MakeSymbolPickerSetCategoryCommand("All"), *model_);
		EndCommandBatch();
	};
	library_tint_.WhenAction = [=] {
		if(!syncing_ && model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetTintCommand((Color)~library_tint_), *model_);
	};
	library_filter_.WhenAction = [=] { ApplyLibraryFilter(); };
	library_filter_.WhenChange = [=] {
		if(syncing_)
			return;
		SetTimeCallback(70, [=] { ApplyLibraryFilter(); }, kLibraryFilterCallbackId);
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
	clear_collection_.WhenAction = [=] { ClearActiveCollection(); };
	collection_filter_.WhenAction = [=] { RefreshCollectionProjection(); };
	collection_filter_.WhenChange = [=] {
		if(syncing_)
			return;
		SetTimeCallback(70, [=] { RefreshCollectionProjection(); }, kCollectionFilterCallbackId);
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
}

void SymbolPickerWorkspaceView::SetModel(SymbolPickerModel* model)
{
	model_ = model;
	seen_library_revision_ = seen_collections_revision_ = seen_export_revision_ = seen_project_revision_ = -1;
	file_actions_.SetContext(model_, catalog_, commands_);
	RefreshFromModel();
}

void SymbolPickerWorkspaceView::SetCatalog(const SymbolPickerCatalog* catalog)
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
		RefreshCollectionSelector();
		RefreshCollectionProjection();
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
	String filter = categories_filter_.GetTextUtf8();
	Vector<SymbolPickerCategory> categories;
	int total = 0;
	if(catalog_) {
		SymbolPickerIconStyle style = model_ ? model_->GetIconStyle() : SymbolPickerIconStyle::Outlined;
		categories = catalog_->GetCategories(style);
		for(const auto& category : categories)
			total += category.icon_count;
	}

	auto AddCategory = [&](const String& id, const String& text) {
		UiButton& button = category_buttons_.Add(new UiButton());
		button.SetText(text).SetCheckable(true).SetChecked(selected == id).SetContentInset(DPI(4));
		categories_flow_.Add(button).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
		button.WhenAction = [=] {
			if(model_ && commands_)
				commands_->Execute(MakeSymbolPickerSetCategoryCommand(id), *model_);
		};
	};

	AddCategory("All", catalog_ ? Format("All (%d)", total) : String("All"));
	for(const auto& category : categories) {
		String text = Format("%s (%d)", category.display_name, category.icon_count);
		if(MatchText(text, filter))
			AddCategory(category.id, text);
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
	library_projection_.Rebuild(*catalog_, model_->GetCurrentCategory(), model_->GetFilterText(), model_->GetIconStyle());
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

	collection_projection_.Rebuild(*active, *catalog_, collection_filter_.GetTextUtf8());
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
	if(UiTheme::GetMode() == UiThemeMode::Dark)
		return White();
	return model_ ? model_->GetTintColor() : Color(Null);
}

Color SymbolPickerWorkspaceView::GetCollectionPreviewTint(const SymbolPickerIconRef& ref) const
{
	return UiTheme::GetMode() == UiThemeMode::Dark ? White() : ref.tint;
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
	for(int i = first; i <= last; ++i) {
		const UiModelItem& current = library_projection_.Model().Get(i);
		if(!current.image.IsEmpty())
			continue;
		const SymbolPickerIconEntry* entry = library_projection_.GetEntry(i);
		if(!entry)
			continue;
		Image image = image_cache_.GetImage(*entry, DPI(kPreviewPx), GetLibraryPreviewTint());
		if(image.IsEmpty())
			continue;
		UiModelItem updated = current;
		updated.image = image;
		library_projection_.Model().Set(i, updated);
	}
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
		Image image = image_cache_.GetImage(*entry, DPI(kPreviewPx), GetCollectionPreviewTint(ref));
		if(image.IsEmpty())
			continue;
		UiModelItem updated = current;
		updated.image = image;
		collection_projection_.Model().Set(i, updated);
	}
	preparing_collection_images_ = false;
}

void SymbolPickerWorkspaceView::UpdateLibraryStatus()
{
	String text = Format("%d symbols", library_projection_.GetCount());
	if(library_gallery_.GetSelectionCount())
		text << Format(" · %d selected", library_gallery_.GetSelectionCount());
	text << Format(" · zoom %.0f%%", library_gallery_.GetZoom() * 100.0);
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
		collections_status_.SetText("No active collection");
		return;
	}
	String text = Format("%s · %d items", active->name, active->items.GetCount());
	if(collection_projection_.GetCount() != active->items.GetCount())
		text << Format(" · %d shown", collection_projection_.GetCount());
	if(collection_gallery_.GetSelectionCount())
		text << Format(" · %d selected", collection_gallery_.GetSelectionCount());
	collections_status_.SetText(text);
}

void SymbolPickerWorkspaceView::UpdateActionState()
{
	const SymbolPickerCollection* active = model_ ? model_->GetActiveCollection() : nullptr;
	bool has_active = active != nullptr;
	bool has_items = has_active && !active->items.IsEmpty();
	remove_collection_.Enable(has_active);
	delete_items_.Enable(has_active && collection_gallery_.GetSelectionCount() > 0);
	clear_collection_.Enable(has_items);
	export_button_.Enable(has_items);
	copy_button_.Enable(has_items);
}

void SymbolPickerWorkspaceView::UpdateThemeButton()
{
	theme_button_.SetText(UiTheme::GetMode() == UiThemeMode::Dark ? "Light" : "Dark");
}

void SymbolPickerWorkspaceView::ApplyLibraryFilter()
{
	if(syncing_ || !model_ || !commands_)
		return;
	commands_->Execute(MakeSymbolPickerSetFilterCommand(library_filter_.GetTextUtf8()), *model_);
}

void SymbolPickerWorkspaceView::CreateCollection()
{
	if(!model_ || !commands_)
		return;
	int new_index = model_->GetCollections().GetCount();
	BeginCommandBatch();
	commands_->BeginGroup("Create collection");
	bool created = commands_->Execute(MakeSymbolPickerCreateCollectionCommand(Format("Collection %d", new_index + 1)), *model_);
	if(created)
		commands_->Execute(MakeSymbolPickerSetActiveCollectionCommand(new_index), *model_);
	commands_->EndGroup();
	EndCommandBatch();
}

void SymbolPickerWorkspaceView::RemoveActiveCollection()
{
	if(!model_ || !commands_ || model_->GetActiveCollectionIndex() < 0)
		return;
	collection_gallery_.ClearSelection();
	BeginCommandBatch();
	commands_->Execute(MakeSymbolPickerRemoveCollectionCommand(model_->GetActiveCollectionIndex()), *model_);
	EndCommandBatch();
}

void SymbolPickerWorkspaceView::ClearActiveCollection()
{
	if(!model_ || !commands_ || model_->GetActiveCollectionIndex() < 0)
		return;
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

void SymbolPickerWorkspaceView::RemoveSelectedCollectionItems()
{
	if(!model_ || !commands_ || model_->GetActiveCollectionIndex() < 0)
		return;
	Vector<int> indexes = GetSelectedCollectionItemIndexes();
	if(indexes.IsEmpty())
		return;
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
		if(!entry)
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
