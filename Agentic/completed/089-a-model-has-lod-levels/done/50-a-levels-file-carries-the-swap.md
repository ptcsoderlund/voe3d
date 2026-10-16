# 50 — A levels file carries the swap, and the store's bounds follow it
folder: 3d
after: 48
decisions: 0168, 0425, 0426, 0430, 0432

## Change
0432 points 1 and 2: `voe_assets_levels` now holds `swap` (`assets/include/assets/parts.h`, card 48).
The levels file records its distance multiplier, and the store divides level bounds by it.

- `3d/include/3d/model_levels.h`: `voe_3d_model_levels` gains `float swap`, the distance
  multiplier (½, 1, 2, 4), commented. Header: THE LEVELS paragraph gains a phrase on the swap (each
  level swaps in at that many times the distance its error allows); THE LAYOUT is version 3, the
  multiplier a float after the part count; version 1 and 2 files are stale; the file records the
  swap, not the count or strength; STALE gains a swap that is not finite and above nought.
  `_make`'s comment: writes `levels.swap`'s multiplier.
- `3d/src/model_levels.c`: `VERSION` 3; a static table from `voe_assets_levels_swap` to 0.5, 1, 2,
  4 beside the strength table, with a `static_assert` on its length; the writer puts the multiplier,
  the reader takes and checks it. Own files carry it too.
- `3d/src/models_levels.c`, `voe_3d_models_levels_upload` and what it calls: the multiplier is the
  file's when `voe_3d_model_levels_read` takes it, else 1 (no file, stale, or a file not read on the
  own path: read it there for the swap alone); every bound of level 1 on, made (0.002 × radius ÷
  error) and own (2^-k), divided by it before the bounds are made non-increasing; infinite stays
  infinite. Header: a phrase on the swap.
- `3d/include/3d/models.h`: the levels paragraph and the `bound` comment say "÷ the file's swap".
- `3d/tests/model_levels.c`: the positional `voe_assets_levels` literals (lines ~114–127) gain
  `VOE_ASSETS_LEVELS_SWAP_NORMAL`. Case 8 (version 1 stale) also checks version 2 stale. New:
  9. the sphere made at SWAP_FAR: read gives `swap` 2; at the default, 1.
  Header lists it.
- `3d/tests/models_levels.c`: a new case: the 64 × 32 sphere loaded with a SWAP_FAR file:
  `levels[1].bound` half (within 1e-6 relative) that of the same sphere loaded with a default
  file. Header lists it.
- `3d/tests/tests.md`: the two entries name the swap in a phrase. `3d/3d.md`: the
  `include/3d/model_levels.h` entry says when they swap, in a phrase.

`game` calls `_make` unchanged; leave it.

## Done when
The tests `3d/model_levels` and `3d/models_levels` pass after the folder's build, with the new
cases in them, and `3d/draw_levels` still passes.
