#ifndef _Utilities_SymbolPicker_SymbolPickerGalleryScaleSmoke_h_
#define _Utilities_SymbolPicker_SymbolPickerGalleryScaleSmoke_h_

#include "SymbolPickerCatalog.h"

namespace Upp {

// Structural regression for the real generated-catalog scale target. It avoids
// timing thresholds and image decoding: the contract is full logical exposure,
// bounded Gallery renderers/paint work, deep navigation, stable data selection,
// and bulk projection notifications.
bool RunSymbolPickerGalleryScaleSmokeTests(const SymbolPickerCatalog& catalog, String& error);

}

#endif
