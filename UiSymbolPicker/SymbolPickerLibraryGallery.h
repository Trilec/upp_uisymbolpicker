#ifndef _Utilities_SymbolPicker_SymbolPickerLibraryGallery_h_
#define _Utilities_SymbolPicker_SymbolPickerLibraryGallery_h_

#include "SymbolPickerGesture.h"
#include <Ui/Ui.h>

namespace Upp {

// UiGallery owns selection, cursor, marquee, zoom and scrolling. This thin
// SymbolPicker adapter adds only the existing drag gesture contract so the
// virtualized library does not need one Ctrl/gesture tracker per logical icon.
class SymbolPickerLibraryGallery : public UiGallery {
public:
	typedef SymbolPickerLibraryGallery CLASSNAME;

	int GetDragIndex() const { return drag_index_; }
	const String& GetDragCatalogId() const { return drag_catalog_id_; }

	Event<> WhenLibraryDragStart;
	Event<Point> WhenLibraryDragMove;
	Event<SymbolPickerGestureResult> WhenLibraryDragFinished;

	virtual void LeftDown(Point p, dword keyflags) override;
	virtual void LeftDrag(Point p, dword keyflags) override;
	virtual void MouseMove(Point p, dword keyflags) override;
	virtual void LeftUp(Point p, dword keyflags) override;
	virtual void CancelMode() override;

private:
	int HitTestVisibleItem(Point p) const;
	bool ProcessDragMove();
	void ResetDrag();

	SymbolPickerGestureTracker drag_gesture_;
	int drag_index_ = -1;
	String drag_catalog_id_;
	bool drag_capture_owned_ = false;
};

}

#endif
