#ifndef _Utilities_SymbolPicker_SymbolPickerLibraryProjection_h_
#define _Utilities_SymbolPicker_SymbolPickerLibraryProjection_h_

#include "SymbolPickerCatalog.h"

namespace Upp {

// Cheap presentation projection for the virtualized symbol library. The domain
// catalog remains authoritative; this object only maps the current filtered
// sequence into the shared UiListModel consumed by UiGallery.
class SymbolPickerLibraryProjection {
public:
	void Clear();
	void Rebuild(const SymbolPickerCatalog& catalog,
		const String& category,
		const String& text,
		SymbolPickerIconStyle style);

	UiListModel& Model() { return model_; }
	const UiListModel& Model() const { return model_; }

	int GetCount() const { return catalog_rows_.GetCount(); }
	int GetCatalogRow(int index) const;
	const SymbolPickerIconEntry* GetEntry(int index) const;
	String GetCatalogId(int index) const;

private:
	const SymbolPickerCatalog* catalog_ = nullptr;
	Vector<int> catalog_rows_;
	UiListModel model_;
};

bool RunSymbolPickerLibraryProjectionSmokeTests(const SymbolPickerCatalog& catalog, String& error);

}

#endif
