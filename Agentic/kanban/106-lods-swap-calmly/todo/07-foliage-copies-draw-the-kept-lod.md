# 07 — Foliage copies draw the kept LOD, and the band goes
folder: 3d
after: 06
decisions: 0168, 0432, 0500, 0501

## Change
Foliage copies choose through `frame->lods` (card 06's field; `3d/include/3d/lods_kept.h`), and the
stateless band (`voe_3d_model_level_choose`, `shown_at`) is removed, nothing calling it after this.

- `3d/src/draw_foliage.c`:
  - `gather_copies`: sort the copy into its tier first. Seen and cast copies choose by
    `voe_3d_lods_kept_choose(frame->lods, voe_3d_lods_kept_copy_key(landscape, kind, x, z), ...)`,
    the landscape the row's entity (`voe_3d_model_entities`, passed down from `gather_row`), x and
    z the copy's painted ones (`voe_assets_landscape_copy`); grown copies key the same way.
    Unseen copies take `voe_3d_model_level` whole and touch no memory (0501 point 5).
  - A dissolving copy is its `level`'s instances dithered by `shown` plus its `from`'s in a `rest`
    group dithered by −`shown`; a settled copy has no `rest` instance. `rest` groups still never
    cast and are not counted.
- `3d/src/draw_foliage.h`: the key's `rest` comment and "A BAND'S `rest` GROUPS DO NOT CAST" say the
  outgoing LOD of a 0.25 s swap, not a band's level before.
- `3d/include/3d/draw_system.h`: the foliage gather comment (~742–755) says the level comes from the
  frame's memory for seen and cast copies, stateless for unseen ones, and the `rest` group holds
  the outgoing LOD while a swap dissolves.
- `3d/include/3d/model_levels.h` and `3d/src/model_levels.c`: remove `voe_3d_model_level_choose` and
  `shown_at`; the choice paragraph ("THE CHOICE HAS NO STATE") becomes: `_size`, `_at` and
  `voe_3d_model_level` are stateless and alike for every asker; a view's kept choice and its
  dissolve live in `lods_kept.h`. The choice struct's comment points there too.
- `3d/tests/model_levels.c`: drop the tests of `_choose` and its band; keep the `voe_3d_model_level`
  ones, and add one `level_at_follows_the_bounds` if no test covers `_at` already.
- `3d/tests/draw_foliage.c`: add `a_copy_swap_dissolves` — one copy, a memory begun each frame: first
  seen near, its kind's level 0 groups and no `rest`; moved past level 1's bound a frame later,
  level 1 groups and a `rest` group at level 0; 0.3 s later level 1 and no `rest`. And
  `unseen_copies_keep_no_memory`: a copy behind the camera and out of every cascade leaves
  `voe_3d_lods_kept_find` false for its key. `3d/tests/tests.md` entry follows.
- `3d/src/src.md`, `3d/include/3d/3d.md`: entries lose the band.

## Meaning
- Believed (observed): foliage dissolves through `rest` groups dithered the other way, drawn by the
  camera and never cast; this card keeps that path and changes only what feeds it.
- Gap answered by 0501 point 4: a painted copy's identity is landscape row, kind and x, z.

## Done when
The tests `3d/draw_foliage`, `3d/model_levels` and `3d/draw_levels` pass, and
`! grep -rq voe_3d_model_level_choose 3d` exits 0.
