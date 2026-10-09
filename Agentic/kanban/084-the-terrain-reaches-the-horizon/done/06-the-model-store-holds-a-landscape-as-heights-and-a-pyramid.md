# 06 — The model store holds a landscape as a heights texture and a pyramid
folder: 3d
after: 02, 03, 05
decisions: 0168, 0379, 0386, 0396

## Change
0396 points 3 to 5: chunk parts, transient chunks and the settle go. Callers in `editor` and
`game` are cards 09 and 10; do not touch them.

- `3d/include/3d/models.h`:
  - Remove `VOE_3D_LANDSCAPE_TRANSIENT_*` and `voe_3d_models_landscape_settle`.
  - Add `VOE_3D_LANDSCAPE_NODES 1024` (nodes a landscape a pass), `VOE_3D_LANDSCAPES_DRAWN 4`
    (landscape rows a frame) and `VOE_3D_LANDSCAPE_WRITE_TEXELS (512u * 512u)` (texels written a
    frame, every landscape together), each saying which capacity a program adds it to.
  - `VOE_3D_MODELS_GEOMETRIES` and its comment count the shared grid as one geometry more,
    as the pictures' quad is.
  - `voe_3d_models_landscape_frame(voe_3d_models *models, voe_render_device *device)`: inside a
    frame, before its first pass: each entry's dirty rect written with
    `voe_render_texture_write_heights`, whole rows of it while the frame's budget lasts, the rest
    kept dirty for the next frame; a refusal keeps it dirty and says so on stderr.
  - The landscape paragraph and `_load_landscape`, `_brush`, `_put` contracts: one part, the
    store's grid wearing the ground; a brush or put updates the pyramid and grows the dirty rect.
- `3d/src/models_store.h`: the held entry loses `statics` and `dirty`; gains the heights texture,
  the `voe_3d_landscape_lod` with the memory it lives in, and a dirty `voe_3d_landscape_rect`. The
  store gains the shared grid geometry: 33 × 33 vertices at x, z in 0..1, normals up, uv = xz, y 0
  but the last row's 1 (so its box is a unit cube; the shader ignores y), 32² · 6 indices. Add an
  internal lookup draw files call: an entry's heights texture, lod and the grid, or none.
- `3d/src/models_landscape.c` (rewrite its header): load creates the texture with
  `voe_render_texture_create_heights` and builds the lod; the grid is made at the first landscape
  load and given back by `voe_3d_models_clear`; part 0 is the grid with `ground()`'s material and
  its faded record; brush, put, frame, saved, rename as above.
- `3d/src/models.c`: the entry free path gives back the texture instead of statics.
- `3d/include/3d/landscape.h`, `3d/src/landscape.c`: remove `VOE_3D_LANDSCAPE_CHUNKS`,
  `voe_3d_landscape_chunk`, `_chunks`, the mesh type and the header's chunk paragraph.
- `3d/tests/landscape.c`: drop the chunk cases. `3d/tests/models_landscape.c`: load gives one
  part and a texture; a brush then `_frame` inside a frame clears the dirt within budget; a
  stamp wider than the budget takes more than one frame; `_put` updates the pyramid as a fresh
  build would. `3d/tests/tests.md`, `3d/include/3d/3d.md`, `3d/src/src.md` follow.

## Done when
`3d/tests/models_landscape.c` and `3d/tests/landscape.c` pass, and
`grep -rn 'LANDSCAPE_TRANSIENT\|landscape_settle\|landscape_chunk' 3d` finds nothing.
