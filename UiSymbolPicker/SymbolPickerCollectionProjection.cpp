#include "SymbolPickerCollectionProjection.h"

namespace Upp {

static bool CollectionProjectionMatches(const String& haystack, const String& filter)
{
	String needle = ToLower(TrimBoth(filter));
	return needle.IsEmpty() || ToLower(haystack).Find(needle) >= 0;
}

void SymbolPickerCollectionProjection::Clear()
{
	item_indexes_.Clear();
	catalog_ids_.Clear();
	model_.Clear();
}

void SymbolPickerCollectionProjection::Rebuild(const SymbolPickerCollection& collection,
	const SymbolPickerCatalog& catalog,
	const String& text)
{
	Vector<int> indexes;
	Vector<String> catalog_ids;
	Vector<UiModelItem> items;
	indexes.Reserve(collection.items.GetCount());
	catalog_ids.Reserve(collection.items.GetCount());
	items.Reserve(collection.items.GetCount());

	for(int i = 0; i < collection.items.GetCount(); ++i) {
		const SymbolPickerIconRef& ref = collection.items[i];
		const SymbolPickerIconEntry* entry = catalog.FindByCatalogId(ref.catalog_id);
		String display = entry ? entry->display_name : String();
		String title = TrimBoth(ref.alias);
		if(title.IsEmpty())
			title = !display.IsEmpty() ? display : ref.catalog_id;
		String haystack = title + "\n" + display + "\n" + ref.catalog_id + "\n" + ref.source_id;
		if(!CollectionProjectionMatches(haystack, text))
			continue;

		UiModelItem item(title, i);
		item.right_text = Format("%d px", ref.size);
		if(ref.unresolved || !entry) {
			item.custom_ink_color = Color(226, 141, 0);
			item.has_metadata = true;
			item.metadata_color = Color(226, 141, 0);
		}
		indexes.Add(i);
		catalog_ids.Add(ref.catalog_id);
		items.Add(pick(item));
	}

	item_indexes_ = pick(indexes);
	catalog_ids_ = pick(catalog_ids);
	model_.Clear();
	model_.AddRange(items);
}

int SymbolPickerCollectionProjection::GetItemIndex(int view_index) const
{
	return view_index >= 0 && view_index < item_indexes_.GetCount() ? item_indexes_[view_index] : -1;
}

String SymbolPickerCollectionProjection::GetCatalogId(int view_index) const
{
	return view_index >= 0 && view_index < catalog_ids_.GetCount() ? catalog_ids_[view_index] : String();
}

bool RunSymbolPickerCollectionProjectionSmokeTests(const SymbolPickerCatalog& catalog, String& error)
{
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	if(catalog.GetIcons().GetCount() < 3)
		return Fail("Collection projection smoke test requires at least three catalog icons.");

	SymbolPickerCollection collection;
	collection.name = "Projection Smoke";
	for(int i = 0; i < 3; ++i) {
		const SymbolPickerIconEntry& entry = catalog.GetIcons()[i];
		SymbolPickerIconRef& ref = collection.items.Add();
		ref.catalog_id = entry.catalog_id;
		ref.source_id = entry.source_id;
		ref.alias = Format("Alias %d", i);
		ref.size = 24 + i * 8;
	}
	SymbolPickerIconRef& missing = collection.items.Add();
	missing.catalog_id = "missing/projection/icon";
	missing.source_id = "missing/projection";
	missing.alias = "Missing Projection";
	missing.unresolved = true;

	SymbolPickerCollectionProjection projection;
	projection.Rebuild(collection, catalog, String());
	if(projection.GetCount() != collection.items.GetCount() || projection.Model().GetCount() != collection.items.GetCount())
		return Fail("Collection projection did not expose all unfiltered items.");
	for(int i = 0; i < projection.GetCount(); ++i) {
		if(projection.GetItemIndex(i) != i || projection.Model().Get(i).data != Value(i))
			return Fail("Collection projection lost underlying item identity.");
	}
	const UiModelItem& unresolved = projection.Model().Get(projection.GetCount() - 1);
	if(!unresolved.has_metadata || IsNull(unresolved.custom_ink_color))
		return Fail("Collection projection did not mark unresolved items.");

	projection.Rebuild(collection, catalog, "Alias 1");
	if(projection.GetCount() != 1 || projection.GetItemIndex(0) != 1)
		return Fail("Collection projection filtering changed underlying item identity.");

	projection.Rebuild(collection, catalog, "Missing Projection");
	if(projection.GetCount() != 1 || projection.GetItemIndex(0) != 3)
		return Fail("Collection projection could not filter an unresolved item.");

	error.Clear();
	return true;
}

}
