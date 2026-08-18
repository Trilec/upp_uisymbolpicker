#include "SymbolPickerCatalog.h"
#include "SymbolPickerCatalogSeed.h"

namespace Upp {

static SymbolPickerIconEntry CopyEntry(const SymbolPickerIconEntry& src)
{
	SymbolPickerIconEntry out;
	out.catalog_id = src.catalog_id;
	out.source_id = src.source_id;
	out.category = src.category;
	out.display_name = src.display_name;
	out.style = src.style;
	out.source_symbol = src.source_symbol;
	out.available = src.available;
	return out;
}

static String CategoryDisplayName(const String& id)
{
	String out;
	bool new_word = true;
	for(int i = 0; i < id.GetCount(); ++i) {
		int c = id[i];
		if(c == '_' || c == '/' || c == '-') {
			out.Cat(' ');
			new_word = true;
			continue;
		}
		out.Cat(new_word ? ToUpper((wchar)c) : c);
		new_word = false;
	}
	return out;
}

static Vector<SymbolPickerCategory> BuildCategories(const Vector<SymbolPickerIconEntry>& icons,
	bool filter_style, SymbolPickerIconStyle style)
{
	VectorMap<String, int> counts;
	for(const auto& icon : icons) {
		if(filter_style && icon.style != style)
			continue;
		counts.GetAdd(icon.category, 0)++;
	}

	Vector<SymbolPickerCategory> out;
	for(int i = 0; i < counts.GetCount(); ++i) {
		SymbolPickerCategory& c = out.Add();
		c.id = counts.GetKey(i);
		c.display_name = CategoryDisplayName(c.id);
		c.icon_count = counts[i];
	}
	Sort(out, [](const SymbolPickerCategory& a, const SymbolPickerCategory& b) {
		return ToLower(a.display_name) < ToLower(b.display_name);
	});
	return out;
}

void SymbolPickerCatalog::Clear()
{
	icons_.Clear();
	catalog_index_.Clear();
	source_index_.Clear();
}

void SymbolPickerCatalog::Add(const SymbolPickerIconEntry& entry)
{
	int index = icons_.GetCount();
	icons_.Add(CopyEntry(entry));

	if(!entry.catalog_id.IsEmpty()) {
		int q = catalog_index_.Find(entry.catalog_id);
		if(q < 0)
			catalog_index_.Add(entry.catalog_id, index);
		else
			catalog_index_[q] = index;
	}
	if(!entry.source_id.IsEmpty() && source_index_.Find(entry.source_id) < 0)
		source_index_.Add(entry.source_id, index);
}

const Vector<SymbolPickerIconEntry>& SymbolPickerCatalog::GetIcons() const
{
	return icons_;
}

Vector<SymbolPickerCategory> SymbolPickerCatalog::GetCategories() const
{
	return BuildCategories(icons_, false, SymbolPickerIconStyle::Outlined);
}

Vector<SymbolPickerCategory> SymbolPickerCatalog::GetCategories(SymbolPickerIconStyle style) const
{
	return BuildCategories(icons_, true, style);
}

Vector<int> SymbolPickerCatalog::Filter(const String& category,
	const String& text,
	SymbolPickerIconStyle style) const
{
	String want_category = TrimBoth(category);
	String want_text = ToLower(TrimBoth(text));
	Vector<int> out;
	for(int i = 0; i < icons_.GetCount(); ++i) {
		const SymbolPickerIconEntry& icon = icons_[i];
		if(style != icon.style)
			continue;
		if(!want_category.IsEmpty() && want_category != "All" && icon.category != want_category)
			continue;
		if(!want_text.IsEmpty()) {
			String hay = ToLower(icon.display_name + " " + icon.source_id + " " + icon.catalog_id + " " + icon.category);
			if(hay.Find(want_text) < 0)
				continue;
		}
		out.Add(i);
	}
	return out;
}

const SymbolPickerIconEntry* SymbolPickerCatalog::FindByCatalogId(const String& catalog_id) const
{
	int q = catalog_index_.Find(catalog_id);
	if(q < 0)
		return nullptr;
	int index = catalog_index_[q];
	return index >= 0 && index < icons_.GetCount() ? &icons_[index] : nullptr;
}

const SymbolPickerIconEntry* SymbolPickerCatalog::FindBySourceId(const String& source_id) const
{
	int q = source_index_.Find(source_id);
	if(q < 0)
		return nullptr;
	int index = source_index_[q];
	return index >= 0 && index < icons_.GetCount() ? &icons_[index] : nullptr;
}

bool RunSymbolPickerCatalogSmokeTests(String& error)
{
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	SymbolPickerCatalog catalog;
	SeedSymbolPickerCatalog(catalog);

	if(catalog.GetIcons().GetCount() != 24)
		return Fail("Catalog did not retain seeded icons.");

	Vector<SymbolPickerCategory> categories = catalog.GetCategories();
	if(categories.IsEmpty())
		return Fail("Catalog categories are empty.");
	bool has_action = false;
	bool has_navigation = false;
	bool has_alert = false;
	for(const auto& category : categories) {
		has_action |= category.id == "action";
		has_navigation |= category.id == "navigation";
		has_alert |= category.id == "alert";
	}
	if(!has_action || !has_navigation || !has_alert)
		return Fail("Catalog categories are missing expected seeded groups.");

	Vector<SymbolPickerCategory> outlined_categories = catalog.GetCategories(SymbolPickerIconStyle::Outlined);
	int outlined_total = 0;
	int outlined_action = 0;
	for(const auto& category : outlined_categories) {
		outlined_total += category.icon_count;
		if(category.id == "action")
			outlined_action = category.icon_count;
	}
	if(outlined_total != 8 || outlined_action != 3)
		return Fail("Catalog style-aware category counts failed.");

	Vector<int> filtered_category = catalog.Filter("action", String(), SymbolPickerIconStyle::Outlined);
	if(filtered_category.GetCount() != 3)
		return Fail("Catalog category filtering failed.");

	Vector<int> filtered_text = catalog.Filter("All", "warn", SymbolPickerIconStyle::Sharp);
	if(filtered_text.GetCount() != 1)
		return Fail("Catalog text filtering failed.");

	Vector<int> filtered_style = catalog.Filter("All", String(), SymbolPickerIconStyle::Rounded);
	if(filtered_style.GetCount() != 8)
		return Fail("Catalog style filtering failed.");

	Index<String> ids;
	for(const auto& icon : catalog.GetIcons()) {
		if(ids.FindAdd(icon.catalog_id) != ids.GetCount() - 1)
			return Fail("Catalog ids are not unique.");
	}

	const SymbolPickerIconEntry *save_outlined = nullptr, *save_rounded = nullptr, *save_sharp = nullptr;
	for(const SymbolPickerIconEntry& icon : catalog.GetIcons()) {
		if(icon.style == SymbolPickerIconStyle::Outlined && !save_outlined)
			save_outlined = &icon;
		if(icon.style == SymbolPickerIconStyle::Rounded && !save_rounded)
			save_rounded = &icon;
		if(icon.style == SymbolPickerIconStyle::Sharp && !save_sharp)
			save_sharp = &icon;
	}
	if(!save_outlined || !save_rounded || !save_sharp)
		return Fail("Catalog fixture is missing style variants.");

	const SymbolPickerIconEntry* by_catalog = catalog.FindByCatalogId(save_rounded->catalog_id);
	if(!by_catalog || by_catalog->catalog_id != save_rounded->catalog_id || by_catalog->style != SymbolPickerIconStyle::Rounded)
		return Fail("Indexed FindByCatalogId did not resolve the requested variant.");
	const SymbolPickerIconEntry* by_source = catalog.FindBySourceId(save_outlined->source_id);
	if(!by_source || by_source->source_id != save_outlined->source_id)
		return Fail("Indexed FindBySourceId did not resolve the requested source.");
	if(catalog.FindBySourceId("missing/id"))
		return Fail("FindBySourceId should return null for missing id.");
	if(catalog.FindByCatalogId("missing/id/outlined"))
		return Fail("FindByCatalogId should return null for missing id.");

	String saved_catalog_id = save_sharp->catalog_id;
	String saved_source_id = save_sharp->source_id;
	catalog.Clear();
	if(!catalog.GetIcons().IsEmpty() || catalog.FindByCatalogId(saved_catalog_id) || catalog.FindBySourceId(saved_source_id))
		return Fail("Catalog Clear() did not clear indexed lookup state.");

	error.Clear();
	return true;
}

}
