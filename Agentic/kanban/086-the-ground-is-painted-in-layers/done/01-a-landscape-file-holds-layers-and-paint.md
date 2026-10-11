# 01 — A landscape file holds layers and paint
folder: assets
after: none
decisions: 0168, 0402

## Change
0402 points 1 and 2: a `.landscape` names up to eight material layers and holds every height
point's weight per layer.

- `assets/include/assets/landscape.h` —
  - `VOE_ASSETS_LANDSCAPE_LAYERS` (8). Include `assets/material.h` for `VOE_ASSETS_MATERIAL_PATH`.
  - `voe_assets_landscape` gains `layers[VOE_ASSETS_LANDSCAPE_LAYERS][VOE_ASSETS_MATERIAL_PATH]`
    (project-relative, NUL-terminated), `layer_count`, and `uint8_t *paint`: two RGBA8 planes of
    (cells + 1)² texels each, row-major as the heights, plane 0 holding layers 0–3 in r g b a and
    plane 1 layers 4–7; every point's eight bytes sum to 255. NULL means every point all layer 0,
    so a struct built by hand (game, 3d tests) stays valid.
  - `voe_assets_landscape_flat` leaves no layers and `paint` NULL.
  - `voe_assets_landscape_resample` carries the layers, and paint when not NULL: each weight
    bilinear at the same point, then layers 1–7 rounded and layer 0 given the rest.
  - `voe_assets_landscape_read`: `[Layers]` `layer0=`… quoted paths, read until the first missing
    key; `[Paint]` keys `paint<k>_<r>=` for k 1..layer_count−1 and every row r, `cells + 1` whole
    numbers 0–255, a token `v*n` meaning n copies of v. With layers and no `[Paint]`, or none at
    all, paint reads as all layer 0 (allocated when there are layers, NULL with none). Layer 0
    is 255 minus the rest. MALFORMED: a path not quoted or too long, a row missing or of the wrong
    count, a value past 255, a point whose layers 1–7 pass 255.
  - `voe_assets_landscape_write`: `[Layers]` when there are any, quoted; `[Paint]` only when paint
    is not NULL and layer_count > 1, rows of layers 1.. only, runs of 3 or more written `v*n`.
  - Header points: the layers and the paint as 0402 says, layer 0 never written, the run token,
    why NULL paint is all layer 0, the example extended.
- `assets/src/landscape.c` — the above. If it passes ~600 lines, put the paint's read and write in
  `assets/src/landscape_paint.c` with its header, declared in a header internal to `src/`.
- `assets/tests/landscape.c` — new cases: a 4-cell grid with three layers and a painted patch
  round-trips byte for byte; a written unpainted layer row is `0*5`; a file with layers and no
  `[Paint]` reads layer 0 = 255 everywhere; a sum past 255 and a row of 4 numbers each MALFORMED;
  a resample of a half-and-half painted grid keeps each point summing to 255 and its corners'
  weights.
- `assets/assets.md`, `assets/src/src.md`, `assets/tests/tests.md` — their landscape entries
  mention layers and paint.

## Done when
Test `assets/landscape` passes with the cases above.
