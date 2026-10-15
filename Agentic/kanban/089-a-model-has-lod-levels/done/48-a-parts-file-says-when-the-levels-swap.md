# 48 — A parts file says when a model's levels swap
folder: assets
after: none
decisions: 0168, 0430, 0432

## Change
0432 point 1: `[Levels]` gains `swap`, how far away the levels swap in. The type lives here so the
levels maker (`3d`), the loader and the editor share it.

- `assets/include/assets/parts.h`:
  - `typedef enum { VOE_ASSETS_LEVELS_SWAP_NEAR, VOE_ASSETS_LEVELS_SWAP_NORMAL,
    VOE_ASSETS_LEVELS_SWAP_FAR, VOE_ASSETS_LEVELS_SWAP_FARTHER } voe_assets_levels_swap;`, its
    comment: written `near`, `normal`, `far`, `farther`; each level swaps in at ½, 1, 2, 4 times the
    distance its error allows (0432 point 1).
  - `voe_assets_levels` gains `voe_assets_levels_swap swap;` after `strength`, commented.
  - `VOE_ASSETS_LEVELS_DEFAULT` gains `VOE_ASSETS_LEVELS_SWAP_NORMAL`.
  - header: the example's `[Levels]` gains `swap="normal"`; the `[Levels]` paragraph gains a phrase
    for `swap` (how far away the levels swap in). `_read`'s comment: MALFORMED also for a swap not
    one of the four words.
- `assets/src/parts.c`: the read takes `swap` as a quoted word as it takes `strength`; the write
  prints it after `strength`. Header: the section's sentence names the swap.
- `assets/tests/parts.c`: the round trip's `levels` (line ~39) becomes `{ 2, STRONG, SWAP_FAR }` and
  still matches byte for byte; the no-`[Levels]` check also reads SWAP_NORMAL; new checks:
  `[Levels]` with only `swap="farther"` reads count 4, MEDIUM, SWAP_FARTHER; `swap="soon"` refused
  MALFORMED. The header lists them.
- `assets/tests/tests.md`, `assets/src/src.md`: the `parts.c` entries name the swap in a phrase.

`3d/tests/model_levels.c` builds `voe_assets_levels` positionally and so warns until card 50
fixes it; leave every other folder.

## Done when
The test `assets/parts` passes after the folder's build, with the new checks in it.
