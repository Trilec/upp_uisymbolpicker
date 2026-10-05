# Maintaining application icons

Symbol Picker maintains a collection of references to artwork, with stable
export names. It does not make each application depend on the picker's source
catalogue or store image data inside its project file.

## Project files

Use `.symbolpicker` for projects. The contents are JSON; the extension makes
the files easy to find in the Load dialog. Older `.uppicons.json` and ordinary
JSON projects remain loadable through the legacy/all-files filters.

- `examples/UiIconsBase.symbolpicker` contains the 71 initially identified Ui
  icons, at 48 px, in one UiIcons application collection.
- `examples/UiIcons.symbolpicker` contains those 71 plus 91 compatibility
  entries. The wider code scan identified 107 explicitly referenced names.
  Retaining all 162 public names avoids removing application dependencies while
  application-owned icons are moved into separate sets.
- `examples/AppIcons.symbolpicker` demonstrates a separate application set.

The full 162-entry shared project also has one UiIcons collection. Original
category metadata remains on each entry; source categories do not split an
application into separate collections. Switching the collection dropdown shows
that application's assets. Icons and raster images can live in the same saved
collection; its All assets / Icons / SVG / Images / PNG view selector only
filters what you see, without changing what is saved or exported.

The installed shared sidecar is `upp_Ui/Ui/UiIcons.symbolpicker`, next to
`UiIcons.h`, `UiIcons.cpp` and `UiIcons.iml`. Load the sidecar, edit the collection,
Save it, and export **U++ IML + Header Library** to `UiIcons.h` in that folder.
The shared project preserves the Ui catalogue/list helpers and exports all
collections together. Keep that filename for this profile. Its default export
type is already set to the paired library.

For a new application, use its own `AppIcons.symbolpicker`, a distinct output
basename and an application prefix such as `APP_`. Save the project beside the
application package. Commit the project, its generated assets, and any external
artwork it references. Applications can use these alongside the shared Ui set.

Projects store catalogue/source IDs, aliases, dimensions, tints, category/style
overrides and usage notes. Imported artwork is stored as a file reference, with
a stable custom ID. Files under the project's directory use relative paths;
files elsewhere use absolute paths. Missing artwork remains visible with a
missing indicator and is reported/skipped by exporters. It is not silently
substituted. Projects contain no SVG or encoded image payloads.

## Adding custom artwork

The library has separate **Icons / SVG** and **Images / PNG** views. The Material
Image category is an icon subject category within Icons / SVG; it is unrelated
to actual PNG/JPEG artwork. The symbol style selector changes Outlined/Rounded/
Sharp in the icon view; raster files have no such style. Imported SVGs and shared
Ui factories remain available independently of Material style.

- Put reusable source assets in `assets/SVG/` or `assets/Images/`. The build
  copies them beside the executable under `assets/`. The picker indexes those
  folders at startup, including subfolders. It renders their contents lazily;
  restarting is enough to discover additions to the distributed folders.
- Subfolders become custom asset categories: `assets/Images/Backgrounds/`
  appears under image categories, while `assets/SVG/Logos/` appears under icon
  categories. The library heading shows the active kind/category/count.
- Use **Import…** to reference SVG, PNG, JPEG, BMP or GIF files elsewhere.
  Save the project to retain those imported references.
- Drop files onto the library to import them. Drop them onto a collection to
  import and add them. Double-click or drag an existing library tile into a
  collection. Repeated source IDs are skipped within that collection.
- SVGs render at the selected export size. Raster images are fitted and
  centred on a square transparent canvas, preserving their aspect ratio.
  Colours are preserved across Light/Dark themes unless a tint is selected.
  Increasing raster dimensions cannot recover missing detail. SVG export
  reports raster-only entries instead of pretending they are vector artwork.
- Changing an external file does not change its ID. Restart the picker or
  import the same file again to refresh its cached preview.

Keep editable source SVGs where possible. The Ui migration rebuilt 137 entries
from exact SVG matches and retained the original artwork for 25 assets without
an exact match. The 71-icon base uses 63 SVG matches and eight original assets.

## Sizes and integration

A useful starting point is a 48 px asset displayed at 24 logical pixels, giving
two physical pixels per logical pixel at 200% scaling. Use 20–24 logical pixels
for ordinary buttons/toolbars; 32 logical pixels may justify a 64 px asset.
These are application choices, not a requirement to display every icon at 48.
Windows' [icon guidance](https://learn.microsoft.com/en-us/windows/win32/uxguide/vis-icons)
also discusses toolbar sizes and DPI variants. Retain SVG masters for future
sizes; an application brand icon has different requirements from a toolbar.

The size selector supplies the size for newly added items. It does not rewrite
items already in a collection. The prepared Ui collection already records 48.

The paired export creates `.h` and `.iml` from the same emission pass. It includes
named image factories, a catalogue of category/display/name/factory entries,
and a category list. For `AppIcons.h`, provide definitions in exactly one `.cpp`:

```cpp
#include <CtrlLib/CtrlLib.h> // Or the application header that includes Ui.
#define APPICONS_IML_IMPLEMENTATION
#include "AppIcons.h"
```

Use `APP_SAVE()` etc. in ordinary code. `GetAPPICONSCatalog(count)` and
`GetAPPICONSCategories(count)` expose the generated catalogue. Shared Ui keeps
`UiIconCatalog()`, `UiIconFromName()`, `UiIconListModel()` and the existing
`ICON_*()` factories. Its `UiIcons.cpp` defines `UIICONS_IML_IMPLEMENTATION`.

Export into the application package directory. The generated `IMAGEFILE`
includes that package directory; the package must be reachable through the
assembly's source nests. If you move the pair to a differently named package,
export it again or update the include path. Add both files to its `.upp` package.

The CLI can perform the same paired export without opening a window:

```powershell
./bin/windows-x64/UiSymbolPicker.exe --export-project examples/AppIcons.symbolpicker MyApp/AppIcons.h
./bin/windows-x64/UiSymbolPicker.exe --project examples/UiIconsBase.symbolpicker
```

## Usage warnings

**Mark used** / **Unmark used** changes selected collection entries. Blue dots
indicate the mark; their button tooltip includes any scan evidence. Deleting,
clearing, or removing a collection with marked entries shows a warning and
allows proceeding. Undo/redo retains the mark and note. It is not a deletion
lock or an automatic dependency analysis.

To collect evidence from code and Designer documents, open **Utilities → Check
icon usage**, choose the saved project and application source folder, scan and
review the matched aliases/file paths, then apply the usage warnings. Save open
collection edits first and reload the project after applying. The utility backs
up the previous file and refuses to overwrite a project changed since scanning.

The same native engine is available for automation:

```powershell
./bin/windows-x64/UiSymbolPicker.exe --scan-usage examples/UiIcons.symbolpicker ../upp_Ui/Ui build/icon-usage
# Add --apply to save the reviewed warnings, or scan a broader common parent.
```

The scan matches literal exported aliases, excludes generated definitions and
project manifests, marks matches, and attaches source paths. It never clears
existing marks. Comments/inactive code may match; dynamic names, unscanned
projects and external callers can be missed. Review an absence of matches
before unmarking an icon. This is native U++ code; Python and rg are not runtime
requirements. Definition files matching the output basename and UiIcons are
excluded automatically.

