#include "SymbolPickerLibraryProjection.h"

namespace Upp {

void SymbolPickerLibraryProjection::Clear()
{
	catalog_ = nullptr;
	catalog_rows_.Clear();
	model_.Clear();
}

void SymbolPickerLibraryProjection::Rebuild(const SymbolPickerCatalog& catalog,
	const String& category,
	const String& text,
	SymbolPickerIconStyle style)
{
	Vector<int> rows = catalog.Filter(category, text, style);
	Vector<UiModelItem> items;
	items.Reserve(rows.GetCount());

	const Vector<SymbolPickerIconEntry>& icons = catalog.GetIcons();
	for(int row : rows) {
		if(row < 0 || row >= icons.GetCount())
			continue;
		const SymbolPickerIconEntry& entry = icons[row];
		UiModelItem item(entry.display_name, entry.catalog_id);
		item.description = entry.category;
		// Images deliberately remain empty here. The Gallery visible-range callback
		// fills only viewport/overscan items through SymbolPickerIconImageCache.
		items.Add(pick(item));
	}

	catalog_ = &catalog;
	catalog_rows_ = pick(rows);
	model_.Clear();
	model_.AddRange(items);
}

int SymbolPickerLibraryProjection::GetCatalogRow(int index) const
{
	return index >= 0 && index < catalog_rows_.GetCount() ? catalog_rows_[index] : -1;
}

const SymbolPickerIconEntry* SymbolPickerLibraryProjection::GetEntry(int index) const
{
	if(!catalog_)
		return nullptr;
	int row = GetCatalogRow(index);
	const Vector<SymbolPickerIconEntry>& icons = catalog_->GetIcons();
	return row >= 0 && row < icons.GetCount() ? &icons[row] : nullptr;
}

String SymbolPickerLibraryProjection::GetCatalogId(int index) const
{
	const SymbolPickerIconEntry* entry = GetEntry(index);
	return entry ? entry->catalog_id : String();
}

bool RunSymbolPickerLibraryProjectionSmokeTests(const SymbolPickerCatalog& catalog, String& error)
{
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	SymbolPickerLibraryProjection projection;
	Vector<int> expected = catalog.Filter("All", String(), SymbolPickerIconStyle::Outlined);
	int revision_before = projection.Model().GetRevision();
	projection.Rebuild(catalog, "All", String(), SymbolPickerIconStyle::Outlined);
	if(projection.GetCount() != expected.GetCount() || projection.Model().GetCount() != expected.GetCount())
		return Fail("Library projection did not expose the complete active-style filter.");
	if(expected.GetCount() > 240 && projection.GetCount() <= 240)
		return Fail("Library projection retained the retired 240-item All cap.");
	if(projection.Model().GetRevision() - revision_before > 2)
		return Fail("Library projection rebuilt with per-item model notifications.");

	for(int i = 0; i < projection.GetCount(); ++i) {
		const SymbolPickerIconEntry* entry = projection.GetEntry(i);
		if(!entry || projection.GetCatalogRow(i) != expected[i])
			return Fail("Library projection lost filtered catalog-row identity.");
		const UiModelItem& item = projection.Model().Get(i);
		if(AsString(item.data) != entry->catalog_id || item.text != entry->display_name)
			return Fail("Library projection did not preserve stable catalog identity.");
		if(!item.image.IsEmpty())
			return Fail("Library projection eagerly prepared icon images.");
	}

	Vector<int> action = catalog.Filter("action", String(), SymbolPickerIconStyle::Outlined);
	projection.Rebuild(catalog, "action", String(), SymbolPickerIconStyle::Outlined);
	if(projection.GetCount() != action.GetCount())
		return Fail("Library projection category rebuild diverged from catalog filtering.");

	return true;
}

}
