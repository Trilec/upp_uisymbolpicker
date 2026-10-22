#ifndef _UiSymbolPicker_Utilities_SymbolPickerMaintenance_h_
#define _UiSymbolPicker_Utilities_SymbolPickerMaintenance_h_

#include <Core/Core.h>
#include <atomic>

namespace Upp {

// Core-only maintenance engine. Callers own cancellation/progress; no GUI access.
struct SymbolPickerSourceRow : Moveable<SymbolPickerSourceRow> {
	String category, name, source, packed_svg;
	int style = 0;
	int optical_size = 24;
	bool filled = false;
};

struct SymbolPickerMaintenanceJob {
	std::atomic<bool> cancelled { false };
	Mutex mutex;
	String progress;
	void Check() const;
	void Status(const String& text);
	String GetStatus();
};

struct SymbolPickerMaintenanceResult {
	ValueMap report;
	String summary, error, snapshot, metadata, stage, project, project_before, project_after;
	bool ok = false;
};

void CompareSymbolPickerSources(const Vector<SymbolPickerSourceRow>& current,
                               const String& source, const String& metadata,
                               const String& output, bool prepare,
                               SymbolPickerMaintenanceJob& job, SymbolPickerMaintenanceResult& result);
void DownloadSymbolPickerSources(const String& remote, const String& output,
                                SymbolPickerMaintenanceJob& job, SymbolPickerMaintenanceResult& result);
void InstallSymbolPickerRefresh(const String& stage, const String& picker_source,
                               SymbolPickerMaintenanceJob& job, SymbolPickerMaintenanceResult& result);
void ScanSymbolPickerUsage(const String& project, const String& code_root,
                          const String& output, SymbolPickerMaintenanceJob& job,
                          SymbolPickerMaintenanceResult& result);
void ApplySymbolPickerUsage(const SymbolPickerMaintenanceResult& scan, const String& output,
                           SymbolPickerMaintenanceJob& job, SymbolPickerMaintenanceResult& result);
bool BuildSymbolPickerRenderVariants(const Vector<SymbolPickerSourceRow>& current, const String& source,
                                    SymbolPickerMaintenanceJob& job, String& header, ValueMap& report);
bool RunSymbolPickerMaintenanceTests(String& error);

}
#endif
