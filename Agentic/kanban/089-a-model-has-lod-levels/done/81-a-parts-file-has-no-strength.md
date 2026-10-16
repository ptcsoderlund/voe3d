# 81 — A parts file has no strength
folder: assets
after: 79
decisions: 0168, 0435, 0437

## Change
0437 points 1 and 3. This changes `assets/parts.h`'s public struct; 3d (card 82) and
editor (83) follow it, so those folders do not build until their cards run. Touch no
file outside `assets/`.

- `assets/include/assets/parts.h`:
  - `voe_assets_levels_strength` and its three values go; `voe_assets_levels` loses
    `strength`; `VOE_ASSETS_LEVELS_DEFAULT` loses its `.strength`.
  - Header comment: the example's `[Levels]` has no `strength` line; the `[Levels]`
    paragraph says an old `strength` key is read and ignored whatever its value and never
    written, and that the edges and collapse limit it set are now fixed in `3d` (0437).
    `_read`'s comment drops the strength MALFORMED case; `_write`'s drops the strength
    assert.
- `assets/src/parts.c`:
  - `strength_words` goes. `read_level_key`: a `strength` key is accepted and ignored,
    whatever its value; its refusal text for an unknown key stops naming strength.
  - `print_parts` writes no `strength=` line; `voe_assets_parts_write` loses the strength
    assert.
  - The header comment and the comment above `read_level_key` say the same.
- `assets/tests/parts.c`:
  - `three_entries_round_trip` loses `.strength` and its check; the expected text has no
    `strength` line. Every other expected text loses it too.
  - The case reading only `strength="gentle"` now checks it reads as the defaults with no
    error, and that writing what it read gives text with no `strength`.
  - `strength="hard"` leaves the MALFORMED list; it reads as the defaults.
  - The header comment says both.
- `assets/tests/tests.md`: the `parts.c` entry, as card 79 left it, if it names strength.
- `assets/src/src.md`: the `parts.c` entry names count, shares and swaps, not strength.

## Done when
The test `assets/parts` passes after the folder's build, and
`grep -rlE 'voe_assets_levels_strength|VOE_ASSETS_LEVELS_(GENTLE|MEDIUM|STRONG)|strength_words' assets`
prints nothing.
