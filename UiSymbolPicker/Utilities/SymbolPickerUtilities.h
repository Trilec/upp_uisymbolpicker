#ifndef _UiSymbolPicker_Utilities_SymbolPickerUtilities_h_
#define _UiSymbolPicker_Utilities_SymbolPickerUtilities_h_
#include <Ui/Ui.h>
namespace Upp {
// The caller owns the modeless window. Closing a running job cancels it safely.
void ShowSymbolPickerUtilities(One<TopWindow>& window, const String& project_path);
bool RunSymbolPickerUtilitiesTests(String& error);
}
#endif
