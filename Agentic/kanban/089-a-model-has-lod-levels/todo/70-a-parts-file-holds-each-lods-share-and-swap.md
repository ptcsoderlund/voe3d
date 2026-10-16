# 70 — A parts file holds each LOD's share and swap
folder: assets
after: none
decisions: 0168, 0435, 0436

## Change
0436 points 1 to 3. This changes `assets/parts.h`'s public struct; 3d (cards 71, 72), game
(73) and editor (74 to 76) follow it, so those folders do not build until their cards run.
Touch no file outside `assets/`.

- `assets/include/assets/parts.h`:
  - `voe_assets_levels_swap` and its words go. `voe_assets_levels` keeps `count` and
    `strength` and gains `float shares[VOE_ASSETS_LEVELS]` (percent of level 0's triangles)
    and `float swaps[VOE_ASSETS_LEVELS]` (multiplier on the distance the error allows),
    index = level as the code counts it; index 0 is always 100 and 1.
  - New `VOE_ASSETS_LEVELS_SWAP_LEAST` 0.1f and `VOE_ASSETS_LEVELS_SWAP_MOST` 10.0f.
  - `VOE_ASSETS_LEVELS_DEFAULT` with designated fields: 6, medium, shares 100, 50, 25, 12,
    6, 3, swaps all 1.
  - Header comment: the example's `[Levels]` shows `share1`…`share5` and `swap1`…`swap5`;
    the `[Levels]` paragraph says what each key is, that N counts from zero as level indices
    do (0434), the ranges and order, that `strength` now only sets edges and how far a
    collapse may go (0435 point 1), and that the old `swap` word is still read but never
    written. `_read`'s and `_write`'s comments name the new MALFORMED cases and asserts.
- `assets/src/parts.c`:
  - `read_level_key`: `share<N>` and `swap<N>`, N 1 to 5, as one number read with `strtof`,
    the whole value consumed (as `number()` in `assets/src/material.c` reads); a share
    outside (0, 100) or a swap outside LEAST to MOST is refused naming the key. The old
    `swap` word (`near`, `normal`, `far`, `farther`) is kept aside as ×0.5, 1, 2 or 4.
  - The `[Levels]` reader, after all keys: the old word fills every `swaps[N]` whose own
    key was not there, whatever the order; then shares 1 to 5 each strictly below the one
    before (share 1 below 100), else MALFORMED naming the first out of order.
  - `print_parts`: after `count` and `strength`, `share1`…`share5` then `swap1`…`swap5`,
    each `%g`; no `swap=` word. `voe_assets_parts_write` asserts each share and swap in
    range and the shares in order, in place of the swap-word assert.
- `assets/tests/parts.c`: every `.swap` use goes. `three_entries_round_trip` uses shares
  80, 40, 10, 5, 2.5 and swaps 3, 1, 0.3, 1, 1, checks them back and that a second write is
  byte-equal. `an_empty_section_reads_count_zero`'s expected text has the ten default keys.
  `absent_levels_read_as_defaults` checks default shares and swaps, and that
  `swap="farther"` reads as swaps 4 on levels 1 to 5. New cases:
  - `old_swap_word_fills_only_absent_swaps`: `swap2=0.5` then `swap="far"`, and the same
    in the other order: swap 2 is 0.5, the others 2.
  - `shares_out_of_order_are_malformed`: `share2=60` alone (above default share 1's 50),
    `share1=100`, `share3=0`, `swap1=0.05`, `swap1=11`, `share6=1`, `swap1=x`: each
    MALFORMED, `out` untouched.
- `assets/tests/tests.md`: the `parts.c` entry names shares and swaps.

## Done when
The test `assets/parts` passes after the folder's build.
