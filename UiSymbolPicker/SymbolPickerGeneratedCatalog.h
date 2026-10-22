#ifndef _Utilities_SymbolPicker_SymbolPickerGeneratedCatalog_h_
#define _Utilities_SymbolPicker_SymbolPickerGeneratedCatalog_h_

#include "SymbolPickerCatalog.h"
#include "Utilities/SymbolPickerMaintenance.h"

namespace Upp {

int LoadGeneratedSymbolPickerCatalog(SymbolPickerCatalog& catalog);
void GetSymbolPickerSourceRows(Vector<SymbolPickerSourceRow>& rows);
bool DecodeGeneratedSymbolPickerSvg(const String& catalog_id, String& svg_xml);
bool RunSymbolPickerVariantTests(const SymbolPickerCatalog& catalog, String& error);
bool RunSymbolPickerGeneratedCatalogSmokeTests(String& error);

}

#endif
