# 05 — The store holds a landscape's layers and paint
folder: 3d
after: 01, 02, 04
decisions: 0168, 0396, 0402

## Change
0402 points 1, 3 and 4, the store's half: a landscape entry owns its paint on the CPU and on the
card, painted and put like heights and written inside a frame.

- `3d/include/3d/models.h` —
  - `voe_3d_models_load_landscape` (and a `.landscape` through `_load`): the entry's grid copy
    carries the layers and always owns paint (all layer 0 when the given one is NULL); two RGBA8
    DATA textures, `VOE_RENDER_SAMPLING_FIELD`, (cells + 1)² each, uploaded from the two planes.
  - `voe_3d_models_landscape_brush`: a PAINT brush stamps the paint (card 04) and grows the
    entry's paint dirt instead of the heights' and pyramid.
  - `voe_3d_models_landscape_put_paint(models, path, rect, const uint8_t *values)`: `rect`'s
    points written into both planes, `values` being plane 0's rect row-major then plane 1's;
    paint dirt grown, entry edited. Nothing for a path that is no loaded landscape.
  - `voe_3d_models_landscape_layer_set(models, path, index, const char *material)`: layer `index`
    takes the path; `index == layer_count` (below 8) adds one, whose weights are nought; entry
    edited. Asserts past that.
  - `voe_3d_models_landscape_frame`: the paint dirt written through
    `voe_render_texture_write_pixels` into both textures in the same shared budget
    (`VOE_3D_LANDSCAPE_WRITE_TEXELS`, its comment saying both kinds share it), the rest carried.
  - `voe_3d_models_rename`: a landscape's layer path equal to `from` or under `from/` follows.
  - `VOE_3D_MODELS_*` device room comments: two more textures per landscape.
  - Header points: the paint owned and its two textures, why FIELD, the shared budget.
- `3d/src/models_store.h` — `entry_held` gains the two paint textures and the paint dirty rect;
  `voe_3d_models_terrain` gains the two paint textures, filled by `voe_3d_models_terrain_of`.
- `3d/src/models_landscape.c` — the above; the textures freed with the entry. If it passes ~600
  lines, move the paint into `3d/src/models_paint.c` with its header.
- `3d/src/models.c` — only the rename's layer paths, if the rename lives there.
- `3d/tests/models_landscape.c` — new cases on a loaded 8-cell landscape with two layers: a
  PAINT brush leaves the heights unchanged and the entry edited; `_landscape_frame` in a frame
  takes the paint dirt and a second frame has none left; `_put_paint` of a rect writes exactly
  those bytes into `entry->landscape->paint`; `_layer_set` at index 2 adds a third layer;
  `_rename` of the layer 1 material's folder follows its path.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — entries follow.

## Done when
Tests `3d/models_landscape` and `3d/landscape` pass.
