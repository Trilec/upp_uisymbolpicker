#ifndef _Utilities_SymbolPicker_SymbolPickerCatalog_h_
#define _Utilities_SymbolPicker_SymbolPickerCatalog_h_

#include "SymbolPickerModel.h"

namespace Upp {

struct SymbolPickerIconEntry : Moveable<SymbolPickerIconEntry> {
	String                catalog_id;
	String                source_id;
	String                category;
	String                display_name;
	String                search_tags;
	SymbolPickerIconStyle style = SymbolPickerIconStyle::Outlined;
	int                   optical_size = 24;
	bool                  filled = false;
	String                source_symbol;
	bool                  available = true;
	UiIconFactoryFn       image_factory = nullptr;
	String                external_file;
	bool                  project_custom = false;
};

struct SymbolPickerCategory : Moveable<SymbolPickerCategory> {
	String id;
	String display_name;
	int    icon_count = 0;
};

// -1: all assets; 0: icon/SVG sources (including shared Ui factories); 1: raster files.
bool SymbolPickerMatchesAssetKind(const SymbolPickerIconEntry& entry, int kind);

class SymbolPickerCatalog {
public:
	void Clear();
	void Add(const SymbolPickerIconEntry& entry);
	void SetCustomAssets(const Vector<SymbolPickerCustomAsset>& assets);

	const Vector<SymbolPickerIconEntry>& GetIcons() const;
	Vector<SymbolPickerCategory> GetCategories() const;
	Vector<SymbolPickerCategory> GetCategories(SymbolPickerIconStyle style, int kind = -1, int optical_size = 24, bool filled = false) const;

	Vector<int> Filter(const String& category,
		const String& text,
		SymbolPickerIconStyle style, int kind = -1, bool include_metadata = true, int optical_size = 24, bool filled = false) const;

	const SymbolPickerIconEntry* FindByCatalogId(const String& catalog_id) const;
	const SymbolPickerIconEntry* FindBySourceId(const String& source_id) const;

private:
	Vector<SymbolPickerIconEntry> icons_;
	VectorMap<String, int> catalog_index_;
	VectorMap<String, int> source_index_;
};

bool RunSymbolPickerCatalogSmokeTests(String& error);

}

#endif
