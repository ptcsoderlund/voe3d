# 11 — The Landscape panel sets the cells a landscape has
folder: editor
after: 10
decisions: 0168, 0379, 0396

## Change
0396 point 2: a Cells box beside Size; a 4 km terrain needs 2048 cells to keep about 2 m a cell.

- `editor/src/models.h` / `editor/src/models.c`:
  - `voe_editor_models_landscape_size_found` becomes `voe_editor_models_landscape_found`, giving
    `size` and `cells`.
  - `voe_editor_models_landscape_size` becomes `voe_editor_models_landscape_shape(models, folder,
    path, float size, uint32_t cells, scratch, why)`: heights stretched to `size` as now and
    resampled to `cells` with `voe_assets_landscape_resample` (card 01) when it differs; the rest
    of the contract (written at once, marked saved, reloaded between frames, no undo) unchanged.
- `editor/src/landscape_panel.h` / `.c`: `_show` takes `cells` too; a second row "Cells" with a
  number box, typed or dragged, rounded to a multiple of 4 and clamped to 4 ..
  `VOE_ASSETS_LANDSCAPE_CELLS_MAX`, handed back once on let go as Size is; the result gains
  `cells`. The header's "one box" and "two rows" points become two boxes and three rows; say
  what the cells are for (detail per metre) and that going down loses detail.
- `editor/src/interface.c` (around lines 182, 488 and 754): the renamed calls, passing the
  cells through.
- `editor/src/src.md`: the panel and models entries.

## Done when
The folder's checks build `voe_editor`, and
`grep -rn 'landscape_size\b\|landscape_size_found' editor/src` finds nothing.

Human: Create → Landscape, set Size 4096 and Cells 2048; the file's `[Landscape]` reads
`size=4096` and `cells=2048`, and the ground is still the same shape, finer.
