#ifndef _Utilities_SymbolPicker_SymbolPickerIconImageCache_h_
#define _Utilities_SymbolPicker_SymbolPickerIconImageCache_h_

#include "SymbolPickerCatalog.h"

namespace Upp {

// Small rendered-symbol cache used by virtualized views. It intentionally keeps
// the current working catalogue instead of running an eviction algorithm on
// every scroll miss: 5,000 28px RGBA previews are only about 15 MB of pixels,
// and Image values shared into UiListModel rows remain cheap references.
class SymbolPickerIconImageCache {
public:
	SymbolPickerIconImageCache();

	void SetMaxEntries(int max_entries);
	Image GetImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint);
	void Clear();

	int GetCount() const { return images_.GetCount(); }
	int GetMaxEntries() const { return max_entries_; }
	int GetHitCount() const { return hit_count_; }
	int GetMissCount() const { return miss_count_; }

private:
	String MakeKey(const SymbolPickerIconEntry& entry, int pixel_size, Color tint) const;
	Image RenderImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint) const;

	VectorMap<String, Image> images_;
	int max_entries_ = 8192;
	int hit_count_ = 0;
	int miss_count_ = 0;
};

bool RunSymbolPickerIconImageCacheSmokeTests(const SymbolPickerCatalog& catalog, String& error);

}

#endif
