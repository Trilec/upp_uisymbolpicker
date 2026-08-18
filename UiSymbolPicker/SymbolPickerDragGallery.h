#ifndef _Utilities_SymbolPicker_SymbolPickerDragGallery_h_
#define _Utilities_SymbolPicker_SymbolPickerDragGallery_h_

#include "SymbolPickerGesture.h"
#include <Ui/Ui.h>

namespace Upp {

// Thin drag adapter over UiGallery. UiGallery remains authoritative for
// selection, marquee, scrolling, zoom and keyboard behavior; this class only
// translates an item press into the SymbolPicker drag gesture payload.
class SymbolPickerDragGallery : public UiGallery {
public:
	typedef SymbolPickerDragGallery CLASSNAME;

	SymbolPickerDragGallery& SetDragSource(SymbolPickerGestureSourceKind source_kind)
	{
		source_kind_ = source_kind;
		return *this;
	}
	SymbolPickerGestureSourceKind GetDragSource() const { return source_kind_; }

	int GetDragViewIndex() const { return drag_view_index_; }
	const String& GetDragCatalogId() const { return drag_catalog_id_; }
	int GetDragCollectionIndex() const { return drag_collection_index_; }

	Event<> WhenDragStart;
	Event<Point> WhenDragMove;
	Event<SymbolPickerGestureResult> WhenDragFinished;

	virtual void LeftDown(Point p, dword keyflags) override;
	virtual void LeftDrag(Point p, dword keyflags) override;
	virtual void MouseMove(Point p, dword keyflags) override;
	virtual void LeftUp(Point p, dword keyflags) override;
	virtual void CancelMode() override;

private:
	int HitTestVisibleItem(Point p) const;
	bool ProcessDragMove();
	void ResetDrag();

	SymbolPickerGestureSourceKind source_kind_ = SymbolPickerGestureSourceKind::Library;
	SymbolPickerGestureTracker gesture_;
	int drag_view_index_ = -1;
	String drag_catalog_id_;
	int drag_collection_index_ = -1;
	bool drag_capture_owned_ = false;
};

}

#endif
