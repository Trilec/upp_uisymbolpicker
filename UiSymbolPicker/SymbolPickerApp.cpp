#include "SymbolPickerApp.h"
#include "SymbolPickerImlExport.h"
#include "SymbolPickerCustomAssets.h"
#include "SymbolPickerLibraryProjection.h"
#include "SymbolPickerCollectionProjection.h"
#include "SymbolPickerGalleryScaleSmoke.h"
#include "SymbolPickerIconImageCache.h"
#include "Utilities/SymbolPickerUtilities.h"

namespace Upp {

static const bool kEnableLocalDemoData = false;

bool SymbolPickerApp::Init(String& error, bool run_tests)
{
	if(run_tests && !RunSymbolPickerMaintenanceTests(error)) return false;
	if(run_tests && !RunSymbolPickerUtilitiesTests(error)) return false;
	if(run_tests && !RunSymbolPickerGestureSmokeTests(error))
		return false;
	if(run_tests && !RunSymbolPickerModelRevisionSmokeTests(error))
		return false;
	if(run_tests && !RunSymbolPickerCommandSmokeTests(error))
		return false;
	if(run_tests && !RunSymbolPickerCatalogSmokeTests(error))
		return false;
	if(run_tests && !RunSymbolPickerGeneratedCatalogSmokeTests(error))
		return false;
	used_seed_fallback_ = LoadGeneratedSymbolPickerCatalog(catalog_) <= 0;
	if(used_seed_fallback_)
		SeedSymbolPickerCatalog(catalog_);
	// Existing Ui assets retain their public names and original artwork.
	for(const auto& icon : UiIconCatalog()) {
		SymbolPickerIconEntry entry;
		entry.catalog_id = "ui/" + icon.name;
		entry.source_id = entry.catalog_id;
		entry.category = "Ui";
		entry.display_name = icon.display_name;
		entry.source_symbol = icon.name;
		entry.image_factory = icon.factory;
		catalog_.Add(entry);
	}
	if(run_tests && !RunSymbolPickerCustomAssetSmokeTests(error)) return false;
	if(run_tests && !RunSymbolPickerLibraryProjectionSmokeTests(catalog_, error))
		return false;
	if(run_tests && !RunSymbolPickerGalleryScaleSmokeTests(catalog_, error))
		return false;
	if(run_tests && !RunSymbolPickerCollectionProjectionSmokeTests(catalog_, error))
		return false;
	if(run_tests && !RunSymbolPickerIconImageCacheSmokeTests(catalog_, error))
		return false;
	if(run_tests && !RunSymbolPickerProjectIoSmokeTests(catalog_, error))
		return false;
	if(run_tests && !RunSymbolPickerExportSmokeTests(catalog_, error))
		return false;
	if(run_tests && !RunSymbolPickerImlExportSmokeTests(catalog_, error))
		return false;
	LoadSymbolPickerAssetFolders(catalog_, AppendFileName(GetExeFolder(), "assets"));
	Wire();
	model_.SetProjectName("SymbolPicker Project");
	if(model_.GetCollections().IsEmpty())
		model_.CreateCollection("Collection 1");
	if(kEnableLocalDemoData)
		SeedDemoData();
	return true;
}

void SymbolPickerApp::Wire()
{
	view_.SetModel(&model_);
	view_.SetCatalog(&catalog_);
	view_.SetCommands(&commands_);
	model_.WhenChanged = [=] {
		view_.RefreshFromModel();
	};
}

void SymbolPickerApp::SeedDemoData()
{
	// Local demo seed only for the shell phase. These values are intentionally
	// obvious placeholders so they do not look like real application defaults.
	model_.CreateCollection("Demo Collection A");
	model_.CreateCollection("Demo Collection B");
	model_.SetActiveCollection(0);
	model_.AddIconToBin("demo/action_placeholder");
	model_.AddIconToBin("demo/alert_placeholder");

	SymbolPickerIconRef unresolved;
	unresolved.catalog_id = "demo/unresolved_placeholder/outlined";
	unresolved.source_id = "demo/unresolved_placeholder";
	unresolved.alias = "DemoMissing";
	unresolved.unresolved = true;
	model_.AddIconToCollection(0, unresolved);
}

void SymbolPickerApp::Run()
{
	view_.Run();
}

}
