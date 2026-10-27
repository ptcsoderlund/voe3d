# 07 — A blending foliage kind blends with its landscape
folder: 3d
after: 04, 06
decisions: 0168, 0481, 0442, 0476, 0388

## Change
0481 points 3 and 5 for foliage: a kind whose look `blends` (card 04) draws its copies blending
with the landscape row they stand on, at its `blend_height` (cm, 0 read as 50).

- `3d/include/3d/draw_system.h` — `voe_3d_foliage_group` gains `const struct voe_3d_ground
  *ground` (forward-declared; NULL draws as ever) and `float ground_reach` in metres. The
  gather's comment: a group is now one (kind, level, part, rest, tier, casts, ground, reach), a
  blending kind's groups naming its row's ground; `voe_3d_draw_foliage_solid`'s span sentence
  in `_run`'s comment if it is told there.
- `3d/src/draw_foliage.h` — `struct voe_3d_foliage_key` gains `ground` and `reach`; the header's
  A GROUP IS paragraph and example follow; `_solid` draws the groups with no ground in the
  `foliage` span, then those with one in a `foliage blend` span opened only when there is one.
- `3d/src/draw_foliage.c` — the gather builds the grounds once with `voe_3d_draw_grounds`
  (`3d/src/draw_ground.h`, card 06) on its arena; a row's ground is the one whose `row` is that
  row's; a copy of a kind whose look `blends` keys its groups by it and the look's reach, a
  row with no painted ground keying none. `_solid` fills each group's record with
  `voe_3d_draw_ground_fill` before its draw. The cast and the bounce draw as ever.
- `3d/src/models_foliage.c`, `3d/include/3d/models_foliage.h` — `_look_set` asserts
  `blend_height` 0 or in the file's range, and its equal-look test counts `blends` and
  `blend_height`; a changed blend remakes as any look change does. The header's ranges sentence follows.
- `3d/src/src.md` — `draw_foliage.*` and `models_foliage.c` entries follow.
- `3d/tests/draw_foliage.c` — a claim added: the same landscape painted, its kind's look set
  blending at 120 cm, every group of that kind has a ground and reach 1.2; at the default look,
  every group's ground is NULL. Its header's claims follow.
- `3d/tests/tests.md` — the `draw_foliage.c` entry follows.

## Done when
`ctest --test-dir build/debug -R '^3d/draw_foliage$'` passes, and the existing 3d foliage and
bounce tests still pass.
