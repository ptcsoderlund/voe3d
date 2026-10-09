# 01 — A landscape holds up to 2048 cells and is resampled to another count
folder: assets
after: none
decisions: 0168, 0379, 0396

## Change
- `assets/include/assets/landscape.h`:
  - Add `VOE_ASSETS_LANDSCAPE_CELLS_MAX 2048`, the most a file may hold (0396 point 1).
    `VOE_ASSETS_LANDSCAPE_CELLS` stays 512 and now means only what Create writes; fix its comment.
  - The struct's `cells` comment, `_flat`'s and `_read`'s contracts say CELLS_MAX instead of 512.
  - Add `voe_assets_landscape voe_assets_landscape_resample(const voe_assets_landscape *from,
    uint32_t cells, voe_base_arena *arena)`: the same size at `cells` (a multiple of 4 from 4 to
    CELLS_MAX, asserted), each new height bilinear from `from`'s at the same point of the square, so
    corners and edges keep their heights and the same count copies exactly.
  - Header points: the cap and why 2048 (2049² inside Vulkan's guaranteed 4096), resample is
    bilinear and loses detail going down.
- `assets/src/landscape.c`: the range check uses CELLS_MAX; add the resample.
- `assets/tests/landscape.c`: a 2048-cell text reads and writes back the same; 2052 is UNSUPPORTED;
  a resample 8 → 16 → 8 of a plane tilted in x and z returns the same heights (bilinear is exact on
  a plane, to 1e-4 m); corners kept; same count is a copy.
- `assets/tests/tests.md`, `assets/include/assets/assets.md` and `assets/assets.md`: the
  landscape entries name the resample and the 2048 cap.

## Done when
`assets/tests/landscape.c` passes with the cases above.
