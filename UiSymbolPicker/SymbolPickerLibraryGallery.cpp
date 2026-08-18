#include "SymbolPickerLibraryGallery.h"

namespace Upp {

int SymbolPickerLibraryGallery::HitTestVisibleItem(Point p) const
{
	UiVisibleRange range = GetVisibleRange(false);
	if(range.IsEmpty())
		return -1;
	for(int i = range.first; i <= range.last; ++i)
		if(GetItemRect(i).Contains(p))
			return i;
	return -1;
}

void SymbolPickerLibraryGallery::LeftDown(Point p, dword keyflags)
{
	int hit = HitTestVisibleItem(p);
	UiGallery::LeftDown(p, keyflags);
	ResetDrag();
	if(hit < 0 || hit >= Model().GetCount())
		return;
	String catalog_id = AsString(Model().Get(hit).data);
	if(catalog_id.IsEmpty())
		return;
	drag_index_ = hit;
	drag_catalog_id_ = catalog_id;
	drag_gesture_.BeginLibrary(catalog_id, GetMousePos());
}

bool SymbolPickerLibraryGallery::ProcessDragMove()
{
	if(!drag_gesture_.IsActive())
		return false;
	Point screen = GetMousePos();
	if(drag_gesture_.ShouldStart(screen, DPI(5)) && drag_gesture_.StartDragging()) {
		SetCapture();
		drag_capture_owned_ = HasCapture();
		if(WhenLibraryDragStart)
			WhenLibraryDragStart();
		return true;
	}
	if(drag_gesture_.IsDragging()) {
		if(WhenLibraryDragMove)
			WhenLibraryDragMove(screen);
		return true;
	}
	return false;
}

void SymbolPickerLibraryGallery::LeftDrag(Point p, dword keyflags)
{
	if(ProcessDragMove())
		return;
	UiGallery::LeftDrag(p, keyflags);
}

void SymbolPickerLibraryGallery::MouseMove(Point p, dword keyflags)
{
	if(ProcessDragMove())
		return;
	UiGallery::MouseMove(p, keyflags);
}

void SymbolPickerLibraryGallery::LeftUp(Point p, dword keyflags)
{
	if(!drag_gesture_.IsActive()) {
		UiGallery::LeftUp(p, keyflags);
		return;
	}

	bool was_dragging = drag_gesture_.IsDragging();
	SymbolPickerGestureResult result = drag_gesture_.Complete(GetMousePos());
	UiGallery::LeftUp(p, keyflags);

	drag_gesture_.BeginOwnedRelease();
	bool do_release = drag_capture_owned_ && HasCapture();
	drag_capture_owned_ = false;
	if(do_release)
		ReleaseCapture();
	drag_gesture_.Reset();
	drag_index_ = -1;
	drag_catalog_id_.Clear();

	if(was_dragging && WhenLibraryDragFinished)
		WhenLibraryDragFinished(result);
}

void SymbolPickerLibraryGallery::CancelMode()
{
	if(drag_gesture_.IsOwnedRelease()) {
		drag_capture_owned_ = false;
		UiGallery::CancelMode();
		return;
	}

	bool was_dragging = drag_gesture_.IsDragging();
	SymbolPickerGestureResult result;
	bool cancelled = drag_gesture_.Cancel(GetMousePos(), result);
	drag_capture_owned_ = false;
	ResetDrag();
	UiGallery::CancelMode();
	if(cancelled && was_dragging && WhenLibraryDragFinished)
		WhenLibraryDragFinished(result);
}

void SymbolPickerLibraryGallery::ResetDrag()
{
	drag_gesture_.Reset();
	drag_index_ = -1;
	drag_catalog_id_.Clear();
	drag_capture_owned_ = false;
}

}
