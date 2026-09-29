# 37 — No light draws black and casts nothing
folder: 3d
after: 35, 36
decisions: 0168, 0287, 0290

## Change
Bug 04, the game half. The owner of "no light" for every picture is `voe_3d_draw_system_light`;
it answers `{ .unshaded = 1 }` today (0238, replaced). It changes once, here; the game's frame,
dev's monitor and the editor all take its answer.

- `3d/src/draw_system.c`: `voe_3d_draw_system_light` returns a zeroed `voe_render_light` when
  the light table has no rows (0290 point 1). Its comment and the one above
  `voe_3d_draw_system_frame`'s body (around line 72) say black, citing 0287, not 0238.
- `3d/src/draw_shadows.c`: the early return (around line 121, "No sun casts nothing") also
  returns, `shadow` zeroed and true, when `frame->light.intensity` is nought: a light of no
  strength casts nothing, and a zeroed light's direction cannot orient cascades (0290 point 2).
  Keep the `unshaded` and `blind` cases.
- `3d/include/3d/draw_system.h`, points to change, in the comments of `voe_3d_draw_system_frame`
  ("with no light the frame is unshaded" and the "AT MOST ONE LIGHT" paragraph),
  `voe_3d_draw_system_light` and `voe_3d_draw_system_shadows` ("With the light shaded"):
  a world with none is still a choice and never asserts; its light is the zeroed one, so lit
  surfaces draw black while unlit materials, text and panels draw as before (0287); it casts no
  shadow; the editor lights such a world with its own preview light by handing its passes a
  different light (0287), which is why `_light` is public. "Directional light" rather than "sun"
  in lines you touch (0288).
- `3d/tests/no_light.c`: the no-light case now checks `unshaded`, `intensity`, `colour` and
  `fill` all nought and the frame not blind; rename `no_light_frames_unshaded` for what it now
  proves; header says black (0287).
- `3d/tests/shadows.c`: the no-light case keeps "no pass, `shadow` zeroed, true" and its two
  pixels are both black, not the floor's colour; header paragraph "NO LIGHT CASTS NOTHING" cites
  0287 and says black.
- `3d/tests/tests.md`: the `no_light.c` and `shadows.c` entries say black.

## Done when
`ctest --test-dir build/debug -R '^3d/(no_light|shadows)$'` passes (after the folder's build),
and `grep -rln 0238 3d/include 3d/src 3d/tests` prints nothing.
