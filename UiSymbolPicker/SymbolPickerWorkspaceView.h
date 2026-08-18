#ifndef _Utilities_SymbolPicker_SymbolPickerWorkspaceView_h_
#define _Utilities_SymbolPicker_SymbolPickerWorkspaceView_h_

#include "SymbolPickerModel.h"
#include "SymbolPickerCatalog.h"
#include "SymbolPickerCommands.h"
#include "SymbolPickerLibraryProjection.h"
#include "SymbolPickerCollectionProjection.h"
#include "SymbolPickerIconImageCache.h"
#include "SymbolPickerFileActions.h"
#include "SymbolPickerDragGallery.h"

#include <Ui/Ui.h>

namespace Upp {

class SymbolPickerWorkspaceDragPreview : public ParentCtrl {
public:
	SymbolPickerWorkspaceDragPreview();
	void SetPreview(const Image& image, int count);
	virtual void Paint(Draw& w) override;
	virtual Size GetMinSize() const override;

private:
	Image image_;
	int count_ = 1;
};

class SymbolPickerWorkspaceView : public TopWindow {
public:
	typedef SymbolPickerWorkspaceView CLASSNAME;

	SymbolPickerWorkspaceView();

	void SetModel(SymbolPickerModel* model);
	void SetCatalog(const SymbolPickerCatalog* catalog);
	void SetCommands(SymbolPickerCommandStack* commands);
	void RefreshFromModel();

	SymbolPickerDragGallery& GetLibraryGallery() { return library_gallery_; }
	SymbolPickerDragGallery& GetCollectionGallery() { return collection_gallery_; }

private:
	void BuildUi();
	void BuildHeader();
	void BuildCategories();
	void BuildLibrary();
	void BuildCollections();
	void WireActions();
	void ConfigureGalleryRenderers();

	void RefreshCategoryButtons();
	void RefreshLibraryProjection(bool preserve_selection = true);
	void RefreshCollectionSelector();
	void RefreshCollectionProjection(bool preserve_selection = true);
	void SyncLibraryControls();
	void SyncExportControls();
	void SyncProjectUi();
	void UpdateLibraryStatus();
	void UpdateCollectionStatus();
	void UpdateActionState();
	void UpdateThemeButton();

	void PrepareLibraryImages(int first, int last);
	void PrepareCollectionImages(int first, int last);
	Color GetLibraryPreviewTint() const;
	Color GetCollectionPreviewTint(const SymbolPickerIconRef& ref) const;
	void EnsureImageGeneration();

	void ApplyLibraryFilter();
	void CreateCollection();
	void RemoveActiveCollection();
	void ClearActiveCollection();
	void RemoveSelectedCollectionItems();
	void AddLibrarySelectionToCollection(const String& primary_catalog_id);
	void ReorderCollectionItem(const SymbolPickerGestureResult& result);
	int GetCollectionDropInsertIndex(Point screen) const;
	String MakeCollectionAlias(const SymbolPickerIconEntry& entry) const;

	Vector<String> GetSelectedLibraryCatalogIds(const String& primary_catalog_id) const;
	Vector<int> GetSelectedCollectionItemIndexes() const;

	void BeginCommandBatch();
	void EndCommandBatch();
	void SetDragActive(bool active);
	void ShowDragPreview(const Image& image, int count, Point screen);
	void MoveDragPreview(Point screen);
	void HideDragPreview();

	virtual bool Key(dword key, int count) override;

	UiBoxLayout root_ { UiDirection::V };

	UiBoxLayout header_ { UiDirection::H };
	UiLabel app_title_;
	UiLabel project_label_;
	UiButton theme_button_;
	UiButton help_button_;
	UiButton exit_button_;

	UiPanel categories_panel_;
	UiBoxLayout categories_box_ { UiDirection::V };
	UiBoxLayout categories_header_ { UiDirection::H };
	UiLabel categories_title_;
	UiLineEdit categories_filter_;
	UiScrollPanel categories_scroll_;
	UiBoxLayout categories_flow_ { UiDirection::H };
	Array<UiButton> category_buttons_;

	UiPanel library_panel_;
	UiBoxLayout library_box_ { UiDirection::V };
	UiBoxLayout library_header_ { UiDirection::H };
	UiBoxLayout library_actions_ { UiDirection::H };
	UiLabel library_title_;
	UiLabel library_status_;
	UiDropdown library_style_;
	UiLabel tint_label_;
	ColorPusher library_tint_;
	UiLineEdit library_filter_;
	SymbolPickerDragGallery library_gallery_;

	UiPanel collections_panel_;
	UiBoxLayout collections_box_ { UiDirection::V };
	UiBoxLayout collections_header_ { UiDirection::H };
	UiBoxLayout collections_actions_ { UiDirection::H };
	UiLabel collections_title_;
	UiLabel collections_status_;
	UiDropdown collections_selector_;
	UiButton new_collection_;
	UiButton remove_collection_;
	UiSplitButton save_button_;
	UiButton load_button_;
	UiSplitButton export_button_;
	UiDropdown export_size_;
	UiDropdown export_type_;
	UiButton copy_button_;
	UiButton delete_items_;
	UiButton clear_collection_;
	UiLineEdit collection_filter_;
	SymbolPickerDragGallery collection_gallery_;

	SymbolPickerWorkspaceDragPreview drag_preview_;

	SymbolPickerLibraryProjection library_projection_;
	SymbolPickerCollectionProjection collection_projection_;
	SymbolPickerIconImageCache image_cache_;
	SymbolPickerFileActions file_actions_;

	SymbolPickerModel* model_ = nullptr;
	const SymbolPickerCatalog* catalog_ = nullptr;
	SymbolPickerCommandStack* commands_ = nullptr;

	bool syncing_ = false;
	bool preparing_library_images_ = false;
	bool preparing_collection_images_ = false;
	bool suppress_refresh_ = false;
	bool refresh_pending_ = false;
	bool drag_active_ = false;

	int seen_library_revision_ = -1;
	int seen_collections_revision_ = -1;
	int seen_export_revision_ = -1;
	int seen_project_revision_ = -1;
	int image_style_ = -1;
	Color image_tint_ = Null;
	bool image_dark_ = false;
	int catalog_generation_ = 0;
	int seen_catalog_generation_ = -1;
};

}

#endif
