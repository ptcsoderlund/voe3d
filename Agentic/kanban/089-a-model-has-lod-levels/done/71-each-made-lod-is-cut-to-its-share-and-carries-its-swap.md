# 71 — Each made LOD is cut to its share and carries its swap
folder: 3d
after: 69, 70
decisions: 0168, 0435, 0436

## Change
0436 points 4 and 5, for made levels; own levels stay as they are until card 72. Follows
card 70's `voe_assets_levels` (`shares[]`, `swaps[]`, no `swap`).

- `3d/src/model_levels.c`:
  - `STRENGTHS` loses `shift`; its comment says strength now sets only the angle and the
    collapse limit. `SWAPS` goes.
  - `simplify_level`: part p's target is its triangles × `shares[k]` ÷ 100, rounded down,
    × 3. The level is dropped, with the ones after, when its triangles are not below
    `previous` (replaces the three-quarters rule). The file's header comment says both.
  - `VERSION` 5; the head's one swap becomes `swaps[1]`…`swaps[5]`, five floats at a fixed
    offset; `HEAD_BYTES` and its comment follow. `write_file`, `take_head` (each swap
    finite and above nought, else stale) and `_make` (copies `levels.swaps`, asserts them
    in `VOE_ASSETS_LEVELS_SWAP_LEAST`…`_MOST`) follow; `swaps[0]` reads as 1.
  - New `voe_3d_model_levels_swaps_set`: writes `swaps[1..5]` into a file's head in place.
- `3d/include/3d/model_levels.h`:
  - `voe_3d_model_levels.swap` becomes `float swaps[VOE_3D_MODEL_LEVELS]`, `swaps[0]` 1.
  - `void voe_3d_model_levels_swaps_set(uint8_t *bytes, size_t size,
    const float swaps[VOE_3D_MODEL_LEVELS])`: asserts the magic, version 5 and room for
    the head, and the swaps in range; its comment says it is how a swap-only change avoids a
    remake (0435 point 4) and that the file stays fresh.
  - Header comment: THE LEVELS says the target per LOD by share, the drop rule and each
    level's own multiplier; THE LAYOUT shows version 5 and five swaps; STALE names a swap
    not finite and above nought. Stay under 60 lines (`checks.sh --header-lines`); a why
    that does not fit moves above its function. `_read`'s comment: version 4 had one swap
    (0436).
- `3d/src/models_levels.c`: `upload_made`'s bound divides by `read->swaps[k]`;
  `upload_own` takes the fresh file's swaps (all 1 without one) and divides 2^-k by
  `swaps[k]`. Its header paragraph EVERY BOUND says per level.
- `3d/include/3d/models.h`: the `.glb` HAS LEVELS paragraph says ÷ the level's swap.
- `3d/tests/model_levels.c`: positional `voe_assets_levels` literals become
  `VOE_ASSETS_LEVELS_DEFAULT` with fields set. Claim 1's three-quarters check becomes
  "each level fewer than the one before". Claim 7 (strong) is dropped: strength no longer
  sets the cut. Claim 15 becomes: swaps 1, 2, 0.5, 1, 1, 1 read back as made; the defaults
  read all 1. New claims, in the header's list:
  - shares 80 and 10 for levels 1 and 2, count 3: level 1's triangles within 10 % of 80 % of
    level 0's, level 2's at most 15 % of level 0's;
  - `_swaps_set` on the defaults' file with swaps 3 on level 2: still reads, `swaps[2]` 3,
    the indices and errors unchanged;
  - the defaults' file with its version set to 4: stale.
- `3d/tests/models_levels.c`: claim 5's `far` sets `swaps[1..5]` to 2 in place of
  `swap`; the claim is unchanged.
- `3d/tests/tests.md`, `3d/src/src.md`, `3d/include/3d/3d.md`: an entry changes only where it
  names the single swap or three quarters.

## Done when
The tests `3d/model_levels` and `3d/models_levels` pass after the folder's build, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/include/3d/model_levels.h`
prints 60 or less.
