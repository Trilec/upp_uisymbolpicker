#include "SymbolPickerDragGallery.h"

namespace Upp {

int SymbolPickerDragGallery::HitTestVisibleItem(Point p) const
{
	UiVisibleRange range = GetVisibleRange(false);
	if(range.IsEmpty())
		return -1;
	for(int i = range.first; i <= range.last; ++i)
		if(GetItemRect(i).Contains(p))
			return i;
	return -1;
}

void SymbolPickerDragGallery::LeftDown(Point p, dword keyflags)
{
	int hit = HitTestVisibleItem(p);
	bool preserve_group = hit >= 0 && IsSelected(hit) && GetSelectionCount() > 1
	                   && !(keyflags & K_CTRL) && !(keyflags & K_SHIFT);
	if(preserve_group)
		SetFocus();
	else
		UiGallery::LeftDown(p, keyflags);

	ResetDrag();
	if(hit < 0 || hit >= Model().GetCount())
		return;

	drag_view_index_ = hit;
	const Value& data = Model().Get(hit).data;
	if(source_kind_ == SymbolPickerGestureSourceKind::Collection) {
		if(!IsNumber(data)) {
			ResetDrag();
			return;
		}
		drag_collection_index_ = (int)data;
		gesture_.BeginCollection(drag_collection_index_, GetMousePos());
	}
	else {
		drag_catalog_id_ = AsString(data);
		if(drag_catalog_id_.IsEmpty()) {
			ResetDrag();
			return;
		}
		gesture_.BeginLibrary(drag_catalog_id_, GetMousePos());
	}
}

bool SymbolPickerDragGallery::ProcessDragMove()
{
	if(!gesture_.IsActive())
		return false;

	Point screen = GetMousePos();
	if(gesture_.ShouldStart(screen, DPI(5)) && gesture_.StartDragging()) {
		SetCapture();
		drag_capture_owned_ = HasCapture();
		if(WhenDragStart)
			WhenDragStart();
		return true;
	}
	if(gesture_.IsDragging()) {
		if(WhenDragMove)
			WhenDragMove(screen);
		return true;
	}
	return false;
}

void SymbolPickerDragGallery::LeftDrag(Point p, dword keyflags)
{
	if(ProcessDragMove())
		return;
	UiGallery::LeftDrag(p, keyflags);
}

void SymbolPickerDragGallery::MouseMove(Point p, dword keyflags)
{
	if(ProcessDragMove())
		return;
	UiGallery::MouseMove(p, keyflags);
}

void SymbolPickerDragGallery::LeftUp(Point p, dword keyflags)
{
	if(!gesture_.IsActive()) {
		UiGallery::LeftUp(p, keyflags);
		return;
	}

	bool was_dragging = gesture_.IsDragging();
	SymbolPickerGestureResult result = gesture_.Complete(GetMousePos());
	UiGallery::LeftUp(p, keyflags);

	gesture_.BeginOwnedRelease();
	bool release_capture = drag_capture_owned_ && HasCapture();
	drag_capture_owned_ = false;
	if(release_capture)
		ReleaseCapture();
	gesture_.Reset();
	drag_view_index_ = -1;
	drag_catalog_id_.Clear();
	drag_collection_index_ = -1;

	if(was_dragging && WhenDragFinished)
		WhenDragFinished(result);
}

void SymbolPickerDragGallery::CancelMode()
{
	if(gesture_.IsOwnedRelease()) {
		drag_capture_owned_ = false;
		UiGallery::CancelMode();
		return;
	}

	bool was_dragging = gesture_.IsDragging();
	SymbolPickerGestureResult result;
	bool cancelled = gesture_.Cancel(GetMousePos(), result);
	drag_capture_owned_ = false;
	ResetDrag();
	UiGallery::CancelMode();
	if(cancelled && was_dragging && WhenDragFinished)
		WhenDragFinished(result);
}

void SymbolPickerDragGallery::ResetDrag()
{
	gesture_.Reset();
	drag_view_index_ = -1;
	drag_catalog_id_.Clear();
	drag_collection_index_ = -1;
	drag_capture_owned_ = false;
}

}
