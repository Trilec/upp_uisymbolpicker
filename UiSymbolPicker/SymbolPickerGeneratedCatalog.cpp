#include "SymbolPickerGeneratedCatalog.h"
#include "SymbolPickerCatalogSeed.h"
#include "Generated/CatalogTables.h"
#include "Generated/SearchTags.h"
#include "Generated/RenderVariants.h"


namespace Upp {

static SymbolPickerIconStyle ToPickerStyleGenerated(int style)
{
	switch(style) {
	case 1: return SymbolPickerIconStyle::Rounded;
	case 2: return SymbolPickerIconStyle::Sharp;
	default: return SymbolPickerIconStyle::Outlined;
	}
}

static String StyleSuffixGenerated(SymbolPickerIconStyle style)
{
	switch(style) {
	case SymbolPickerIconStyle::Rounded: return "rounded";
	case SymbolPickerIconStyle::Sharp:   return "sharp";
	default:                             return "outlined";
	}
}

static String MakeDisplayNameGenerated(const char* raw_name)
{
	String out;
	bool new_word = true;
	for(const char* s = raw_name; *s; ++s) {
		if(*s == '_' || *s == '-' || *s == '/') {
			out.Cat(' ');
			new_word = true;
			continue;
		}
		int c = (byte)*s;
		out.Cat(new_word ? ToUpper((wchar)c) : c);
		new_word = false;
	}
	return out;
}

template <class T>
static String MakeCatalogIdGenerated(const T& row)
{
	String source_id = String(row.category) + "/" + row.name;
	return source_id + "/" + StyleSuffixGenerated(ToPickerStyleGenerated((int)row.style));
}

template <class T>
static int AddGeneratedRows(SymbolPickerCatalog& catalog, const T* rows, int count)
{
	int added = 0;
	for(int i = 0; i < count; ++i) {
		const T& row = rows[i];
		SymbolPickerIconEntry e;
		e.category = row.category;
		e.display_name = MakeDisplayNameGenerated(row.name);
		e.search_tags = SymbolPickerSearchTags().Get(row.name, String());
		e.source_id = String(row.category) + "/" + row.name;
		e.style = ToPickerStyleGenerated((int)row.style);
		e.catalog_id = MakeCatalogIdGenerated(row);
		e.source_symbol = row.source ? String(row.source) : String();
		e.available = row.b64zIcon && *row.b64zIcon;
		catalog.Add(e);
		++added;
	}
	return added;
}

template <class T>
static bool DecodeGeneratedRowSvg(const T* rows, int count, const String& catalog_id, String& svg_xml)
{
	for(int i = 0; i < count; ++i) {
		const T& row = rows[i];
		if(MakeCatalogIdGenerated(row) != catalog_id)
			continue;
		if(!row.b64zIcon || !*row.b64zIcon)
			return false;
		String packed = Base64Decode(String(row.b64zIcon));
		if(packed.IsEmpty())
			return false;
		svg_xml = ZDecompress(packed);
		return !svg_xml.IsEmpty();
	}
	return false;
}

struct GeneratedPayloadRow : Moveable<GeneratedPayloadRow> {
	String category;
	String name;
	String source;
	String catalog_id;
	SymbolPickerIconStyle style = SymbolPickerIconStyle::Outlined;
	int optical_size = 24;
	bool filled = false;
	const char* b64z = nullptr;
};

static Vector<GeneratedPayloadRow>& GeneratedRowsStorage()
{
	static Vector<GeneratedPayloadRow> rows;
	return rows;
}

static VectorMap<String, int>& GeneratedLookupStorage()
{
	static VectorMap<String, int> lookup;
	return lookup;
}

template <class T>
static void AppendGeneratedIndexRows(const T* rows, int count)
{
	Vector<GeneratedPayloadRow>& out = GeneratedRowsStorage();
	VectorMap<String, int>& lookup = GeneratedLookupStorage();
	for(int i = 0; i < count; ++i) {
		const T& row = rows[i];
		GeneratedPayloadRow& item = out.Add();
		item.category = row.category;
		item.name = row.name;
		item.source = row.source ? String(row.source) : String();
		item.catalog_id = MakeCatalogIdGenerated(row);
		item.style = ToPickerStyleGenerated((int)row.style);
		item.b64z = row.b64zIcon;
		lookup.Add(item.catalog_id, out.GetCount() - 1);
	}
}

static void EnsureGeneratedLookup()
{
	Vector<GeneratedPayloadRow>& rows = GeneratedRowsStorage();
	if(!rows.IsEmpty())
		return;

#define INDEX_TABLE(ns) AppendGeneratedIndexRows(ns::kIcons, ns::kIconCount);
	SYMBOLPICKER_GENERATED_ICON_TABLES(INDEX_TABLE);
#undef INDEX_TABLE
	VectorMap<String, int> variants;
	for(int i = 0; kSymbolPickerRenderVariants[i].name; ++i) {
		const auto& row = kSymbolPickerRenderVariants[i];
		variants.Add(String(row.name) + "/" + AsString(row.style) + "/" + AsString(row.optical) + (row.filled ? "/filled" : "/unfilled"), i);
	}
	int legacy_count = rows.GetCount();
	for(int i = 0; i < legacy_count; ++i) {
		GeneratedPayloadRow base = clone(rows[i]);
		for(int optical : {20, 48}) for(bool filled : {false, true}) {
			String suffix = "/" + AsString(optical) + (filled ? "/filled" : "/unfilled");
			int q = variants.Find(base.name + "/" + AsString((int)base.style) + suffix);
			if(q < 0) continue;
			const auto& payload = kSymbolPickerRenderVariants[variants[q]];
			GeneratedPayloadRow item = clone(base);
			item.optical_size = optical; item.filled = filled; item.catalog_id << suffix;
			item.source = "symbols/web/" + base.name + "/materialsymbols" + StyleSuffixGenerated(base.style) + "/" + base.name
				+ (filled ? "_fill1_" : "_") + AsString(optical) + "px.svg";
			item.b64z = payload.packed;
			GeneratedLookupStorage().Add(item.catalog_id, rows.GetCount()); rows.Add(pick(item));
		}
	}

}

void GetSymbolPickerSourceRows(Vector<SymbolPickerSourceRow>& output)
{
	EnsureGeneratedLookup();
	output.Clear();
	for(const auto& row : GeneratedRowsStorage()) {
		auto& item = output.Add();
		item.category = row.category;
		item.name = row.name;
		item.source = row.source;
		item.style = (int)row.style;
		item.optical_size = row.optical_size; item.filled = row.filled;
		item.packed_svg = row.b64z;
	}
}

int LoadGeneratedSymbolPickerCatalog(SymbolPickerCatalog& catalog)
{
	catalog.Clear();
	EnsureGeneratedLookup();

	const Vector<GeneratedPayloadRow>& rows = GeneratedRowsStorage();
	for(const auto& row : rows) {
		SymbolPickerIconEntry e;
		e.category = row.category;
		e.display_name = MakeDisplayNameGenerated(row.name);
		e.search_tags = SymbolPickerSearchTags().Get(row.name, String());
		e.source_id = row.category + "/" + row.name;
		e.style = row.style;
		e.optical_size = row.optical_size; e.filled = row.filled;
		e.catalog_id = row.catalog_id;
		e.source_symbol = row.source;
		e.available = row.b64z && *row.b64z;
		catalog.Add(e);
	}
	return rows.GetCount();
}

bool DecodeGeneratedSymbolPickerSvg(const String& catalog_id, String& svg_xml)
{
	svg_xml.Clear();
	EnsureGeneratedLookup();
	const VectorMap<String, int>& lookup = GeneratedLookupStorage();
	int q = lookup.Find(catalog_id);
	if(q < 0)
		return false;
	const GeneratedPayloadRow& row = GeneratedRowsStorage()[lookup[q]];
	if(!row.b64z || !*row.b64z)
		return false;
	String packed = Base64Decode(String(row.b64z));
	if(packed.IsEmpty())
		return false;
	svg_xml = ZDecompress(packed);
	return !svg_xml.IsEmpty();
}

bool RunSymbolPickerGeneratedCatalogSmokeTests(String& error)
{
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	SymbolPickerCatalog generated;
	int count = LoadGeneratedSymbolPickerCatalog(generated);
	if(count <= 0)
		return Fail("Generated catalog loader returned no rows.");
	if(generated.GetIcons().IsEmpty())
		return Fail("Generated catalog is empty.");

	Index<String> catalog_ids;
	bool has_outlined = false;
	bool has_rounded = false;
	bool has_sharp = false;
	bool has_action = false;
	bool has_navigation = false;
	bool has_content = false;
	String sample_catalog_id;

	for(const auto& icon : generated.GetIcons()) {
		if(icon.catalog_id.IsEmpty())
			return Fail("Generated catalog produced an empty catalog_id.");
		if(icon.source_id.IsEmpty())
			return Fail("Generated catalog produced an empty source_id.");
		if(catalog_ids.FindAdd(icon.catalog_id) != catalog_ids.GetCount() - 1)
			return Fail("Generated catalog ids are not unique.");
		if(!icon.available)
			return Fail("Generated catalog row was unexpectedly marked unavailable.");
		has_outlined |= icon.style == SymbolPickerIconStyle::Outlined;
		has_rounded |= icon.style == SymbolPickerIconStyle::Rounded;
		has_sharp |= icon.style == SymbolPickerIconStyle::Sharp;
		has_action |= icon.category == "action";
		has_navigation |= icon.category == "navigation";
		has_content |= icon.category == "content";
		if(sample_catalog_id.IsEmpty())
			sample_catalog_id = icon.catalog_id;
	}

	if(!has_outlined || !has_rounded || !has_sharp)
		return Fail("Generated catalog is missing one or more style variants.");
	if(!has_action || !has_navigation || !has_content)
		return Fail("Generated catalog is missing expected categories.");

	for(SymbolPickerIconStyle style : { SymbolPickerIconStyle::Outlined, SymbolPickerIconStyle::Rounded, SymbolPickerIconStyle::Sharp }) {
		bool found = false;
		for(int i : generated.Filter("All", "grip", style))
			found |= generated.GetIcons()[i].source_id.EndsWith("/drag_indicator");
		if(!found) return Fail("Metadata search did not find Drag Indicator by grip in every style.");
		for(int i : generated.Filter("All", "grip", style, -1, false))
			if(generated.GetIcons()[i].source_id.EndsWith("/drag_indicator"))
				return Fail("Disabling metadata still matched Drag Indicator by grip.");
		if(generated.Filter("All", "drag_indicator", style, -1, false).IsEmpty())
			return Fail("Name-only search did not match Drag Indicator.");
	}

	String svg_xml;
	if(sample_catalog_id.IsEmpty() || !DecodeGeneratedSymbolPickerSvg(sample_catalog_id, svg_xml) || svg_xml.IsEmpty())
		return Fail("Generated catalog could not decode a sample SVG payload.");
	if(!IsSVG(svg_xml))
		return Fail("Generated catalog sample payload did not decode to SVG.");

	SymbolPickerCatalog fallback;
	SeedSymbolPickerCatalog(fallback);
	if(count > 0 && generated.GetIcons().GetCount() == fallback.GetIcons().GetCount())
		return Fail("Generated catalog smoke test looks like seeded fallback data.");

	return true;
}

}
