# UiSymbolPicker

**UiSymbolPicker is a U++ desktop tool for browsing, organising and exporting icon libraries, including reusable IML and C++ header generation.**

It browses generated symbol/icon catalogues, builds persistent icon collections,
and produces deterministic application assets without making visual tiles the
authority for asset identity.

## Purpose

- browse generated symbol/icon catalogues;
- filter by category/style/name;
- select individual or multiple symbols;
- organise symbols into reusable collections;
- save/load SymbolPicker projects;
- export usable U++ assets.

## Model-driven large-library architecture

The generated catalog currently contains 15,171 entries: 5,057 source symbols
across Outlined, Rounded and Sharp variants. The application exposes one active
style at a time, so an unfiltered library contains about 5,057 logical items.

The production library and active collection use `UiGallery` with shared
`UiListModel` presentation projections. Logical item count is independent of
live renderer/control count:

```text
SymbolPickerCatalog / SymbolPickerModel
                ↓
     presentation projection
                ↓
           UiListModel
                ↓
             UiGallery
                ↓
 visible + overscan UiItemRenderImage pool
```

There is no one-`Ctrl`-per-symbol population and no 240-item cap. Gallery uses
uniform-grid arithmetic for visible ranges, scrolling, marquee and zoom; it does
not need a Graph-style spatial index.

SVG previews are rendered lazily for the visible/overscan range and shared via a
simple bounded image cache. Catalog ID/source ID lookups are indexed, and model
revision channels keep unrelated export/project changes from rebuilding the
large library projection.

`UiModelItem.data` carries stable catalog identity, so selection remains semantic
rather than depending on filtered row numbers.

See `docs/ACTIVE_WORK.md` and `docs/SUPERVISOR_LIBRARY_DISPLAY_HANDOFF.md` for
the current scale contract and validation state.

## Main exports

- Image calls;
- icon IDs;
- C++ snippets;
- PNG;
- SVG;
- U++ RAW headers;
- U++ RLE headers;
- standalone U++ IML;
- paired U++ IML + Header Library.

The paired `U++ IML + Header Library` export writes a sibling `.iml` + `.h` pair
from one deterministic emission pass, so the header wrappers/catalogue cannot
drift from the image IDs actually emitted into the IML. It is intended for
maintaining reusable libraries such as:

```text
UiIcons.iml
UiIcons.h
```

To provide the IML image definitions without a third source file, define the
generated `<BASENAME>_IML_IMPLEMENTATION` macro in exactly one translation unit
before including the generated header; the header then pulls in `iml_source.h`
for that one translation unit.

## Dependencies

- Ultimate++ / U++ (`uppsrc`)
- `upp_Ui` — the `Ui` control library package
- `Utilities/IconExportCore` — U++ IML packing helpers (inside `upp_Ui`)
- `upp_animation` and `upp_statemachine` — supporting U++ packages used by the
  application's dependency graph

The dependency direction is one-way:

```text
upp_uisymbolpicker
        ↓
      upp_Ui
```

This repository does not vendor `upp_Ui` source; it builds against it as an
external dependency via the local assembly.

## Repository structure

```text
UiSymbolPicker/   production application
tests/            deterministic validation packages
examples/         integration/examples
docs/             active work, handoff and changelog
build/            local build output, not versioned
github.var        local U++ assembly definition
```

## Building on the supplied Windows setup

The local assembly is `github.var`. From the repository root:

```bat
E:\upp-18468\umk.exe github UiSymbolPicker CLANGx64 -r +GUI E:\apps\github\upp_uisymbolpicker\build\UiSymbolPicker.exe
```

The application compiles entirely from this repository's `UiSymbolPicker`
sources plus the declared external dependencies; nothing is compiled from the
old `upp_Ui\Utilities\SymbolPicker` package.

> Note: the package currently builds with blitz disabled (release mode `-r`
> rather than `-br`). A pre-existing static helper collision inside the package
> (`EscapeCppString`) breaks the single-translation-unit blitz compile in both
> the standalone repository and the original `upp_Ui` source tree, so the
> faithful build is the non-blitz one shown above.

### Smoke / header tests

Build the deterministic header fixtures:

```bat
E:\upp-18468\umk.exe github SmokeHeaderGenerate CLANGx64 -r +CONSOLE E:\apps\github\upp_uisymbolpicker\build\SmokeHeaderGenerate.exe
E:\upp-18468\umk.exe github SmokeHeaderCompile CLANGx64 -r +GUI E:\apps\github\upp_uisymbolpicker\build\SmokeHeaderCompile.exe
```

Verify the generated fixtures match the committed ones under `tests\SmokeHeaderCompile`:

```bat
E:\apps\github\upp_uisymbolpicker\build\SmokeHeaderGenerate.exe E:\apps\github\upp_uisymbolpicker\tests\SmokeHeaderCompile --verify
```

Expected deterministic identities:

```text
RAW catalog_id: action/camera_enhance/outlined
RLE catalog_id: action/generating_tokens/outlined
```

Then compile-check the fixtures (validates size, visible pixels, and
premultiplied RGBA):

```bat
E:\apps\github\upp_uisymbolpicker\build\SmokeHeaderCompile.exe
```

## Licence

See the repository `LICENSE` (GNU GPLv3).
