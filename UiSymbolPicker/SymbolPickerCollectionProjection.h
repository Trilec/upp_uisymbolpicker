#ifndef _Utilities_SymbolPicker_SymbolPickerCollectionProjection_h_
#define _Utilities_SymbolPicker_SymbolPickerCollectionProjection_h_

#include "SymbolPickerCatalog.h"
#include "SymbolPickerModel.h"

namespace Upp {

// Presentation-only projection of the active collection. It keeps the domain
// collection authoritative and exposes only the filtered visual order plus the
// underlying item index needed by commands/reordering.
class SymbolPickerCollectionProjection {
public:
	void Clear();
	void Rebuild(const SymbolPickerCollection& collection,
		const SymbolPickerCatalog& catalog,
		const String& text);

	UiListModel& Model() { return model_; }
	const UiListModel& Model() const { return model_; }

	int GetCount() const { return item_indexes_.GetCount(); }
	int GetItemIndex(int view_index) const;
	String GetCatalogId(int view_index) const;

private:
	Vector<int> item_indexes_;
	Vector<String> catalog_ids_;
	UiListModel model_;
};

bool RunSymbolPickerCollectionProjectionSmokeTests(const SymbolPickerCatalog& catalog, String& error);

}

#endif
