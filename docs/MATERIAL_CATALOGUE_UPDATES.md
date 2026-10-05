# Refreshing the Material Symbols catalogue

The source of truth is a pinned SVG snapshot, its live category metadata,
and a report comparing them with the generated headers. Refreshing does not
require the application to parse thousands of SVG files at startup.

## Included catalogue

The included catalogue contains 4,150 symbol names in outlined, rounded and sharp
styles. Category memberships can overlap, so category totals exceed the unique
name count. The pinned source commit and file hashes are retained in
`UiSymbolPicker/Generated/provenance.json`.

The source design is the regular, unfilled 24 px SVG. Filled variants and other
optical sizes are separate designs. SVG exports can be rendered at larger pixel
sizes without enlarging an existing bitmap.

## Native utility workflow

Open **Utilities → Catalogue updates** in Symbol Picker. The utility always
inherits the app's theme; it has no independent theme setting. Its UiDoc help
explains each field and action.

1. Choose an existing **Material download** containing `symbols/web`, or choose
   **Download latest**. The default is the Trilec Material Design Icons fork. Downloads create
   a fresh folder and record the fetched commit; they do not overwrite an
   existing third-party checkout. Git on PATH is required for download only.
2. Download also fetches category metadata from Google Fonts over Windows HTTPS.
   An existing metadata JSON can be selected instead. Metadata is optional for
   comparison and required for preparing new category entries.
3. **Compare** checks baseline names, all three styles, canonical SVG artwork,
   unreadable files, retired entries and live-metadata discrepancies against
   the catalogue compiled into this executable. The full JSON report records
   evidence; an existing folder is not proof of a current upstream download.
4. **Prepare refresh** writes C++ headers into a separate working folder.
   Existing category/name/style IDs survive, retired SVG payloads are retained,
   and new names use live categories or Uncategorized. Incomplete styles or
   unreadable files block staging. The receipt hashes every prepared file.
5. Review the report, then **Install prepared refresh**. It validates the receipt,
   backs up the current generated files and package, copies the prepared files,
   and registers new category headers in the `.upp` file. Failures restore the
   previous files. Cancellation is accepted before the installation transaction.
6. Rebuild and open the new `bin/windows-x64/UiSymbolPicker.exe`. Installation
   changes source files; the running executable's catalogue remains unchanged.

Build the `UiSymbolPicker` package in the Ultimate++ IDE as described in the
[README](../README.md#building-from-source).

The same native engine can be used without a window:

```powershell
./bin/windows-x64/UiSymbolPicker.exe --download-material https://github.com/Trilec/material-design-icons.git build/material-download
./bin/windows-x64/UiSymbolPicker.exe --compare-material build/material-next build/material-symbols-metadata.json build/material-audit --prepare
# Use '-' in place of metadata for a comparison without category metadata.
./bin/windows-x64/UiSymbolPicker.exe --install-refresh build/material-audit/prepare-<id>/Generated . build/material-install
```

Each operation writes its detailed report under the selected working folder.
CLI runs also write `last-result.json` and return a nonzero exit status on failure.
No Python or rg runtime is required. Automatic metadata download currently uses
Windows WinHTTP; other platforms can select downloaded metadata instead.

Compilation verifies generated C++ tables. Gallery checks verify each
category/style's reachable final item with a bounded renderer pool. Projects
retain stable source IDs through refreshes.

## Cost and caching

The native update tool enumerates symbol directories and reads the selected baseline SVGs,
validates them and compares XML. It writes compressed Base64 SVG payloads and
SHA-256 provenance. This work happens during maintenance.
The main library does not scan 12,450 loose SVG files each time it opens.

At runtime, compiled metadata builds an indexed catalogue. SVG decoding and
image rendering happen only when needed. Previews share a bounded image cache;
the gallery allocates controls for the visible/overscan range. Normal startup only loads catalogue metadata and the previews it needs.

Custom assets in `assets/SVG` and `assets/Images` are an independent smaller
folder library. Startup enumerates their filenames, and rendering remains lazy.
Do not copy the entire upstream repository into those folders: keep the large
Material catalogue compiled and the custom folders for development assets.

