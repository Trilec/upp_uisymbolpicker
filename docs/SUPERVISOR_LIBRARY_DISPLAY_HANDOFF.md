# Supervisor Hand-off: SymbolPicker library icon display issue

Status: handed off. Working tree is clean at HEAD `6a87f91` (which includes the
`EscapeCppString` BLITZ fix). The app builds and opens in this state, but the
underlying library-display issue below is NOT yet fixed.

## What the user reported

- With category **All** selected, only ~220-270 icons appear, but the catalog has
  thousands.
- Selecting **action** shows "Action (1716)" on the button but far fewer icons
  in the grid.
- The library status line showed "572 icons" while the grid appeared to show
  fewer.

## Root cause (confirmed with a probe against the real generated data)

The catalog is large and counts are reported in two different ways:

1. **Total catalog**: 15,171 entries = 5,057 source icons x 3 style variants
   (Outlined / Rounded / Sharp). Each generated header (e.g.
   `UiSymbolPicker/Generated/icons_action.h`) contains 572 rows per style, so
   action = 1,716 entries across all styles but only **572 per style**.

2. **`SymbolPickerCatalog::Filter(...)` only ever returns ONE style** (the model
   holds a single `SymbolPickerIconStyle`, default Outlined). So "action +
   Outlined" = 572 rows, "All + Outlined" = 5,057 rows.

3. **The view hard-caps the "All" case** in `SymbolPickerView.cpp`
   (`RebuildLibraryTiles`):

   ```cpp
   static constexpr int kLibraryAllInitialLimit = 240;
   bool limited_all = category == "All" && text.IsEmpty() && rows > 240;
   int visible_count = limited_all ? min(rows, 240) : rows;
   ```

   So selecting **All** only ever builds 240 tiles, even though the filter
   matches 5,057. Status read "Showing 240 of 5057 icons". This is the main
   "missing icons" bug.

4. **Category buttons count all styles** (`GetCategories()` counts every entry
   regardless of style) so "Action (1716)" is misleading: the view will only
   ever show 572 for that category.

## What was tried (all reverted)

1. Removed the `kLibraryAllInitialLimit = 240` cap so "All" builds the full
   5,057-tile set.
2. Added `SymbolPickerCatalog::GetCategories(SymbolPickerIconStyle)` and used
   it in the view so buttons show per-style counts (e.g. "Action (572)",
   "All (5057)").
3. Made preview image decoding lazy (batch timer, decode visible tiles first),
   because eager decoding of 5,057 SVGs at startup was also too slow.

### Result: the cap removal exposed a second, larger problem

With the cap removed, **the window never appears**: the app pins a CPU core and
no window handle is created for 90+ seconds. Cause: the very first library
rebuild happens **inside `SymbolPickerApp::Init()`** — `Wire()` connects
`model_.WhenChanged` to `view_.RefreshFromModel()`, and `SetProjectName(...)`
in `Init` fires `WhenChanged` → `RefreshFromModel` → `RebuildLibraryTiles`,
which runs **before `Run()` ever shows the window**. Building 5,057 tiles
(each `SymbolPickerIconTile` = a `ParentCtrl` with two `Label` children) and
laying them out in the `UiBoxLayout` Flow grid, plus their SVG decodes, blocks
for 30-90s before the window appears.

Lazy image decoding alone did NOT fix it: tile construction + Flow layout of
5,057 items is itself too slow to do eagerly at startup. The prior 240-tile
build was fast, which is why the capped version "comes up pretty quick".

## Recommendation for the supervisor

The correct fix is to make the library grid **virtualized** so the number of
created tiles is independent of catalog size:

- Keep the full filtered row list (`catalog_->Filter(...)`) as the authoritative
  set and derive the total scroll extent from its count.
- Only create `SymbolPickerIconTile` controls for the visible viewport range
  (plus a small buffer), reusing/rebuilding them on scroll, instead of building
  all tiles eagerly in `RebuildLibraryTiles`.
- Keep preview image decoding lazy (decode only for visible tiles; cache them).
- Fix the "All" cap: remove `kLibraryAllInitialLimit` once virtualization makes
  the count irrelevant, OR keep a small eager cap + "load more" only if
  virtualization is deferred.
- Make category button counts style-aware so the button number matches what the
  grid shows (`GetCategories(SymbolPickerIconStyle)`), and make the "All"
  button count consistent too.

## Files involved

- `UiSymbolPicker/SymbolPickerView.cpp` — `RebuildLibraryTiles`,
  `RebuildCategoryButtons`, `kLibraryAllInitialLimit`, `UpdateLibraryStatus`.
- `UiSymbolPicker/SymbolPickerView.h` — library tile members.
- `UiSymbolPicker/SymbolPickerCatalog.h/.cpp` — `GetCategories()` /
  `Filter(...)`.
- Startup path: `UiSymbolPicker/SymbolPickerApp.cpp` `Init()`/`Wire()`.

## Current state

- HEAD `6a87f91`, working tree clean, app builds and opens (with the 240-cap
  behavior and the misleading all-style counts).
- All experimental changes above were reverted. `git` history has no trace of
  them (they were uncommitted).
