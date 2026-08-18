# ACTIVE WORK — UiSymbolPicker

## Repository truth

Repository: `Trilec/upp_uisymbolpicker`

Authoritative branch: `main`

Current source checkpoint: `36a5c77c9cc843edb393f54921ae8a88f167a0a8`

Remote GitHub is authoritative. Refresh `main` before continuing and do not reset it backwards.

## Active objective

Finish Windows build/runtime acceptance of the model-driven SymbolPicker workspace that replaces the old capped per-icon Ctrl/Flow library.

The source migration is complete. Do not redesign the library back around per-item controls or pagination.

## Why this work existed

The generated catalog contains 15,171 entries: 5,057 source symbols across Outlined, Rounded and Sharp variants. The UI presents one style at a time, so `All + style` is approximately 5,057 logical library items.

The retired view hard-capped `All` at 240 because removing the cap made startup unusable: it eagerly created thousands of `ParentCtrl` tiles, child Labels, Flow-layout entries and rendered SVG previews before the window opened.

The catalog size was not the problem. The eager view architecture was.

## Current architecture

### Domain authority

- `SymbolPickerCatalog` owns the complete generated catalog.
- `SymbolPickerModel` owns application/domain state: style, category, filter, tint, collections, project and export settings.
- `SymbolPickerCommandStack` remains the mutation/undo authority.

### Presentation projections

- `SymbolPickerLibraryProjection` maps the current catalog filter into one `UiListModel` plus O(1) filtered-index -> catalog-row lookup.
- `SymbolPickerCollectionProjection` maps the active collection into one `UiListModel` plus underlying collection-item indexes.
- Projection models are view data only. They do not replace the domain model/catalog.

### Views

- `SymbolPickerWorkspaceView` is the production TopWindow.
- Library and active collection are both `SymbolPickerDragGallery` / `UiGallery` surfaces.
- Ordinary symbols are `UiItemRenderImage` presentations, not one Ctrl per logical item.
- `UiGallery` owns selection, Ctrl/Shift selection, marquee, keyboard navigation, zoom, scrolling and the bounded visible renderer pool.
- Category buttons remain ordinary controls because there are only a few dozen; virtualizing them would add complexity without benefit.

### Scale behavior

- No 240-item cap exists in the production source path.
- `All + active style` exposes the complete filtered model (~5,057 items on the generated catalog).
- Gallery geometry is uniform-grid arithmetic; no graph-style spatial hash is required.
- Only visible + overscan `UiItemRender` instances exist.
- Deep scrolling does not instantiate preceding symbols.
- `SymbolPickerGalleryScaleSmoke` structurally checks the real generated catalog for 5k exposure, bounded renderers/Paint work, final-item navigation, stable selection tokens and bulk projection notifications.

### Image preparation/cache

- Projection rows begin with no rendered image.
- `UiGallery::WhenVisibleRange` prepares images only for the useful visible/overscan range.
- `SymbolPickerIconImageCache` caches rendered images by catalog ID + pixel size + tint.
- Cache storage is a simple `VectorMap<String, Image>`; the old eviction scan/reindex machinery is gone.
- The cache ceiling is hard-bounded to 8,192 entries. This comfortably covers one 5,057-item style while preventing the 15,171 generated variants from becoming an accidental permanent image archive.
- Catalog identity lookups are indexed (`catalog_id` and `source_id`) rather than repeated full-catalog scans.

### Scoped model refresh

`SymbolPickerModel` exposes monotonic revisions for library, collections, export and project state. `SymbolPickerWorkspaceView` rebuilds only the affected projection/control group. Export-only changes do not rebuild the 5k library.

### Dragging

`SymbolPickerDragGallery` is a thin reusable adapter over `UiGallery`. Gallery remains authoritative for selection/marquee/scroll/zoom. The adapter only adds the existing SymbolPicker drag gesture payload and capture-safe completion.

Library drag uses Gallery selection as the drag set. Collection drag maps the filtered Gallery row back to the underlying collection item for reorder.

### File/export behavior

Project save/load and export dialogs/emission were extracted from the old view into `SymbolPickerFileActions`. Presentation code no longer owns these large file-action paths.

## Removed legacy code

Removed from production/repository:

- `SymbolPickerView.h/.cpp`
- `SymbolPickerResponsiveLayout.cpp`
- `SymbolPickerIconTile`
- `SymbolPickerCollectionTile`
- library `UiScrollPanel + UiBoxLayout` per-item Flow population
- `kLibraryAllInitialLimit = 240`
- old capped-result state
- obsolete `SymbolPickerLibraryGallery` adapter

Do not restore these to solve a view problem.

## Published migration checkpoints

- `8f4feb9423fdab19b1a6c3e198785efd2155ebe1` — style-aware category counts
- `5b4defa873f2536751f68ce202bfbb501acdc0ca` — full library projection
- `3623c289e41cec718bf0c0429f02a04e7fde8e38` — simple lazy rendered-image cache
- `3a3769f74eaa4243e2730826e2249ebb6d9bcb00` — projection row/model lockstep cleanup
- `f7c520535a33b2757ac7a7f60440e5c8149969bd` — collection projection + generic Gallery drag adapter
- `7065f6433d4e543630129a9511ea39096bf1d39b` — project/export file actions extracted
- `78e6df4621702bffb70deadc021d59188a5233dc` — scoped model revision channels
- `259041d978e6da68f8d6061553dc5887d39e1cf6` — indexed catalog identity lookup
- `b9b3fe3e35dacca7a32c67ea9f4f877728b4378d` — production app switched to Gallery workspace
- `08020420745adcde261a59e74b6314126fa171a4` — structural 5k Gallery scale regression
- `52565abbf5dfab6bc652c497fc7276485c6eb9a2` — retired tile/Flow view removed
- `36a5c77c9cc843edb393f54921ae8a88f167a0a8` — obsolete adapter removed; cache ceiling enforced

There is one harmless historical commit `7ab7cf1...` in main containing an accidentally created empty `dummy` file. The immediately following forward commit removed it; the current tree does not contain that file. Do not rewrite history.

## Validation status

Source review: complete for the migration slice.

Windows compile/runtime acceptance: **PENDING** for the new production workspace.

Do not claim the 5k migration accepted until Gary builds the current `main` against current `upp_Ui/main`, launches the app, and performs the bounded smoke below.

## First Windows gate

1. Build `UiSymbolPicker` Debug and Release (non-BLITZ if the existing package build still requires it).
2. Launch Debug; all startup smoke tests must pass and the window must appear promptly.
3. Confirm `All + Outlined` exposes approximately 5,057 symbols, not 240.
4. Scroll/jump to the final symbol without a large pre-show or deep-scroll stall.
5. Confirm Debug status shows renderer count bounded near the viewport rather than logical item count.
6. Verify images fill lazily during navigation and previously seen images reuse the cache.
7. Switch Outlined/Rounded/Sharp and verify style-aware category counts.
8. Test search, Ctrl/Shift/marquee selection and Gallery zoom.
9. Drag multiple selected library symbols into the active collection.
10. Reorder and delete collection items.
11. Light/Dark, undo/redo, basic Save/Load and one text export smoke.
12. `git diff --check` and clean status.

Gary may repair only simple mechanical compiler issues (missing include, exact overload/signature, package membership). Architecture/performance/ownership problems return to the supervisor.

## Known follow-up candidates — measure before changing

- Gallery currently emits semantic selection changes during marquee movement. Grid arithmetic keeps candidate lookup cheap; only change this to transient-preview/commit-on-release if the 5k workspace shows measurable event churn.
- Visible image preparation currently updates useful model rows individually. The useful range is bounded. If Windows profiling shows this causes scroll hitching, the next surgical improvement is a shared `UiListModel` bulk-range update rather than app-side caching tricks or another view store.
- Dark mode currently favors white symbol previews for legibility. Revisit explicit tint-vs-theme precedence only if manual UX acceptance shows the chosen tint should remain visible in Dark mode.

Do not add a spatial tree to Gallery. Uniform grid arithmetic is the correct broad-phase for this control.
