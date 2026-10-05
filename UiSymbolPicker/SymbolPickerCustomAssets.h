#ifndef _SymbolPickerCustomAssets_h_
#define _SymbolPickerCustomAssets_h_
#include "SymbolPickerCatalog.h"

namespace Upp {
bool IsSymbolPickerAssetFile(const String& path);
void LoadSymbolPickerAssetFolders(SymbolPickerCatalog& catalog, const String& folder);
bool RunSymbolPickerCustomAssetSmokeTests(String& error);
}
#endif
