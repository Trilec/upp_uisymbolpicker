#include "SymbolPickerGalleryScaleSmoke.h"
#include "SymbolPickerLibraryProjection.h"

#include <Ui/Ui.h>

namespace Upp {

bool RunSymbolPickerGalleryScaleSmokeTests(const SymbolPickerCatalog& catalog, String& error)
{
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	Vector<int> expected = catalog.Filter("All", String(), SymbolPickerIconStyle::Outlined);
	if(expected.IsEmpty())
		return Fail("Gallery scale smoke requires at least one outlined symbol.");

	const bool full_generated_catalog = catalog.GetIcons().GetCount() >= 5000;
	if(full_generated_catalog && expected.GetCount() < 5000)
		return Fail("Gallery scale smoke did not expose the expected 5k+ active-style catalog.");

	SymbolPickerLibraryProjection projection;
	int model_changes = 0;
	projection.Model().WhenChange << [&](const UiModelChange&) { ++model_changes; };
	projection.Rebuild(catalog, "All", String(), SymbolPickerIconStyle::Outlined);
	if(projection.GetCount() != expected.GetCount() || projection.Model().GetCount() != expected.GetCount())
		return Fail("Gallery scale smoke projection count differs from the full active-style filter.");
	if(full_generated_catalog && projection.GetCount() <= 240)
		return Fail("Gallery scale smoke detected the retired 240-item cap.");
	if(model_changes > 1)
		return Fail("Gallery scale smoke initial projection emitted per-item model notifications.");

	UiGallery gallery;
	gallery.SetModel(projection.Model())
	       .SetSelectionMode(UIGALLERYSEL_MULTI)
	       .SetItemSize(Size(DPI(92), DPI(92)))
	       .SetGap(DPI(6))
	       .SetInset(DPI(6))
	       .SetOverscanRows(2);
	gallery.SetRect(0, 0, DPI(900), DPI(520));
	gallery.Layout();

	UiVisibleRange overscan = gallery.GetVisibleRange(true);
	UiVisibleRange visible = gallery.GetVisibleRange(false);
	if(overscan.IsEmpty() || visible.IsEmpty())
		return Fail("Gallery scale smoke produced an empty visible range.");
	if(gallery.GetLiveItemRenderCount() <= 0 || gallery.GetLiveItemRenderCount() > overscan.GetCount())
		return Fail("Gallery scale smoke renderer pool is not bounded by the useful range.");
	if(full_generated_catalog && gallery.GetLiveItemRenderCount() >= projection.GetCount() / 4)
		return Fail("Gallery scale smoke renderer pool grew with logical catalog size.");

	ImageDraw paint(DPI(900), DPI(520));
	gallery.Paint(paint);
	if(gallery.GetLastPaintItemCount() <= 0 || gallery.GetLastPaintItemCount() > visible.GetCount())
		return Fail("Gallery scale smoke painted outside the viewport-sized logical range.");

	int last = projection.GetCount() - 1;
	gallery.SetCursor(last);
	UiVisibleRange deep = gallery.GetVisibleRange(false);
	if(gallery.GetCursor() != last || deep.IsEmpty() || !deep.Contains(last))
		return Fail("Gallery scale smoke could not deep-scroll to the final logical symbol.");

	int stable_index = min(100, projection.GetCount() - 1);
	gallery.SetCursor(stable_index);
	String stable_id = projection.GetCatalogId(stable_index);
	Value stable_token = gallery.GetData();
	int before_rebuild_changes = model_changes;
	projection.Rebuild(catalog, "All", String(), SymbolPickerIconStyle::Outlined);
	int rebuild_changes = model_changes - before_rebuild_changes;
	if(rebuild_changes > 2)
		return Fail("Gallery scale smoke projection rebuild emitted per-item model notifications.");
	gallery.SetData(stable_token);
	if(gallery.GetSelectionCount() != 1 || projection.GetCatalogId(gallery.GetCursor()) != stable_id)
		return Fail("Gallery scale smoke did not restore selection by stable catalog identity.");

	error.Clear();
	return true;
}

}
