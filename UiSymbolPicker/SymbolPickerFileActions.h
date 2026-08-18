#ifndef _Utilities_SymbolPicker_SymbolPickerFileActions_h_
#define _Utilities_SymbolPicker_SymbolPickerFileActions_h_

#include "SymbolPickerModel.h"
#include "SymbolPickerCatalog.h"
#include "SymbolPickerCommands.h"
#include "SymbolPickerExport.h"

namespace Upp {

// File/project/export operations shared by the application shell. Keeping these
// outside the view avoids coupling persistence/export logic to any particular
// list or Gallery presentation.
class SymbolPickerFileActions {
public:
	void SetContext(SymbolPickerModel* model,
		const SymbolPickerCatalog* catalog,
		SymbolPickerCommandStack* commands);

	bool SaveProject(bool save_as = false);
	bool LoadProject();
	bool CopyCurrentExportToClipboard();
	bool Export(SymbolPickerExportScope scope);

private:
	String BuildProjectDialogTitle(const char* verb) const;
	String MakeExportDefaultExtension() const;
	String MakeExportDefaultName(SymbolPickerExportScope scope) const;
	String BuildExportText(SymbolPickerExportScope scope, Vector<String>* warnings = nullptr) const;
	void ValidateLoadedProject(SymbolPickerProject& project) const;
	void ShowExportComplete(const String& file_name, const String& folder, const String& detail = String()) const;
	bool ExportText(SymbolPickerExportScope scope);
	bool ExportImlLibrary(SymbolPickerExportScope scope);
	bool ExportSvgFiles(SymbolPickerExportScope scope);
	bool ExportPngFiles(SymbolPickerExportScope scope);

	SymbolPickerModel* model_ = nullptr;
	const SymbolPickerCatalog* catalog_ = nullptr;
	SymbolPickerCommandStack* commands_ = nullptr;
};

}

#endif
