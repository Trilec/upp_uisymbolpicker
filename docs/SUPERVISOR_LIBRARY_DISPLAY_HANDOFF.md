# Supervisor Hand-off: SymbolPicker model-driven Gallery baseline

Status: **source migration complete; Windows acceptance pending**.

Current source checkpoint at handoff: `36a5c77c9cc843edb393f54921ae8a88f167a0a8`.

Always refresh remote `main` before continuing; GitHub is authoritative.

## Problem that triggered the migration

The generated catalog contains 15,171 entries: 5,057 source icons with Outlined, Rounded and Sharp variants. SymbolPicker presents one style at a time, so `All + style` should expose about 5,057 logical items.

The retired view capped `All` at 240 because its architecture created one `ParentCtrl` tile (plus child Labels and gesture state) per logical icon inside a wrapping `UiBoxLayout`. Removing the cap caused thousands of controls and Flow-layout entries to be built before the window opened. Lazy SVG decode alone could not fix that.

The model/catalog size was not the bottleneck. The eager Ctrl-per-item view was.

## Implemented replacement

The production app now uses `SymbolPickerWorkspaceView`.

### Library

`SymbolPickerCatalog`
→ `SymbolPickerLibraryProjection`
→ `UiListModel`
→ `SymbolPickerDragGallery` / `UiGallery`
→ bounded visible `UiItemRenderImage` pool.

The complete active-style filter is exposed. There is no 240-item cap.

`UiModelItem.data` stores stable `catalog_id`, so Gallery selection survives projection rebuilds by semantic identity rather than filtered row number.

### Collections

The active collection uses the same model/view principle:

`SymbolPickerCollection`
→ `SymbolPickerCollectionProjection`
→ `UiListModel`
→ `SymbolPickerDragGallery`.

Filtered view rows retain the underlying collection item index for delete/reorder commands.

### Image work

Rendered SVG previews are not built with the projection. `UiGallery::WhenVisibleRange` prepares only visible + overscan rows.

`SymbolPickerIconImageCache` is a simple bounded `VectorMap<String, Image>` keyed by catalog ID, preview size and tint. It has an 8,192-entry hard ceiling, large enough for one 5,057-item style without becoming an all-variants archive.

Catalog identity lookup is indexed instead of repeatedly scanning 15k entries.

### Refresh scope

`SymbolPickerModel` has monotonic library/collections/export/project revisions. The workspace rebuilds only the affected projection/control group. Export changes do not rebuild the 5k library.

### Interaction

`UiGallery` owns selection, Ctrl/Shift selection, marquee, keyboard navigation, scrolling and zoom. `SymbolPickerDragGallery` only adds the existing drag gesture payload and capture-safe completion.

Do not add Graph's spatial hash to Gallery. Gallery's uniform grid gives cheaper direct row/column arithmetic.

### Project/export

Save/load/export dialog and file-emission orchestration moved to `SymbolPickerFileActions`; the workspace is no longer a giant mixed presentation/file-action class.

## Structural 5k regression

`SymbolPickerGalleryScaleSmoke` runs against the real loaded generated catalog and checks structure rather than wall-clock timing:

- generated active style exposes >=5,000 rows;
- no 240 cap;
- renderer pool is bounded by visible + overscan range;
- Paint work is viewport-sized;
- deep navigation reaches the final logical symbol;
- stable catalog selection token survives projection rebuild;
- projection rebuild uses bulk notifications rather than one notification per logical item.

## Removed legacy source

The repository no longer contains the old production path:

- `SymbolPickerView.h/.cpp`
- `SymbolPickerResponsiveLayout.cpp`
- `SymbolPickerIconTile`
- `SymbolPickerCollectionTile`
- per-icon ScrollPanel/Flow population
- `kLibraryAllInitialLimit`
- the superseded library-only `SymbolPickerLibraryGallery` adapter.

Do not restore these for compatibility; the app is still new and the Gallery model/view path is canonical.

## Important published checkpoints

- `8f4feb94...` style-aware category counts
- `5b4defa8...` library projection
- `3623c289...` lazy/simple image cache
- `f7c52053...` collection projection + generic Gallery drag adapter
- `7065f643...` file actions extracted
- `78e6df46...` scoped model revisions
- `259041d9...` indexed catalog identity
- `b9b3fe3e...` production app switched to Gallery workspace
- `08020420...` structural 5k regression
- `52565abb...` retired tile view removed
- `36a5c77c...` superseded adapter removed; image-cache ceiling hardened

See `docs/ACTIVE_WORK.md` for exact recovery state and validation checklist.

## Next action

Do **not** redesign further before the first Windows build. Gary should build/launch current main against current `upp_Ui/main` and report mechanical compiler issues separately from substantive architecture/runtime findings.

Acceptance must prove:

- prompt startup;
- ~5,057 `All + Outlined` logical symbols;
- bounded renderer count;
- smooth deep scrolling and lazy image fill;
- style-aware counts/search;
- multi-selection/marquee/zoom;
- library-to-collection multi-drag;
- collection reorder/delete;
- Light/Dark and undo/redo;
- basic Save/Load/export smoke;
- clean Git hygiene.

If visible image row updates show measurable scroll hitching, the next optimization should be a small shared `UiListModel` bulk-range update. Do not create another semantic cache/store or reintroduce item Ctrls.
