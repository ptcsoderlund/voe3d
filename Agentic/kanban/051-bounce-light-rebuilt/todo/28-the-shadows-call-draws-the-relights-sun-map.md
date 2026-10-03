# 28 — The shadows call fits and draws the relight's sun map
folder: 3d
after: 25, 27
decisions: 0168, 0328, 0329

## Change
0329 points 2 and 3, as 3d sees them; the build of 3d is whole again after this card (card 27
removed `voe_render_bounce_frame.shadow`).
- `3d/include/3d/bounce_grid.h`, `3d/src/bounce_grid.c`: `voe_render_view
  voe_3d_bounce_grid_sun(voe_3d_bounce_grid grid, voe_math_double3 eye, voe_math_float3 direction)`:
  the light view of the relight's map, a sphere about the volume's centre (`corner` plus half its
  sides) of radius half its diagonal plus `VOE_RENDER_BOUNCE_REACH`, texel 2 × radius over
  `VOE_RENDER_BOUNCE_SHADOW_TEXELS`, through `3d/src/light_box.h`'s basis, snap and look as the
  cascades use them. Header: it holds every surface a probe can see; the snap moves the map by
  whole texels when the volume scrolls; it does not depend on the view.
- `3d/src/draw_shadows.c`: `light_casts` becomes `voe_3d_draw_light_casts`, declared in
  `3d/src/draw_bounce.h` as `voe_3d_draw_casts` is.
- `3d/src/draw_bounce.c`, `3d/src/draw_bounce.h`: the begin's record without `shadow`; after the
  capture passes, when the light casts, `voe_render_bounce_shadow_pass_begin` with the sun view of
  the fitted grid and the frame's light direction; when opened, `voe_3d_draw_casters` into it and the
  pass closed; then the relight. False when a pass or a draw is refused. Headers say so.
- `3d/include/3d/draw_system.h`: `voe_3d_draw_system_shadows`' bounce paragraph: one more pass, and
  one more object per caster, on a frame that relights a casting sun; the relight's sun shadow is its
  own map, not the cascades (0329).
- `3d/tests/bounce_grid.c`: the sun view: the volume's centre lands in the map's middle within a
  texel and its eight corners inside it, for a sun at 45 degrees and straight down.
- `3d/tests/bounce.c`: `ALL_PASSES` counts the bounce shadow pass; frame two opens four capture
  passes and the bounce shadow pass (true with exactly that many `passes`, false with one fewer); a
  sun that does not cast opens no bounce shadow pass.
- `3d/tests/bounce_scene.c`: a full frame's passes and `DUMMIES` count the bounce shadow pass; a
  LOOKING AWAY case after card 25's TURN, standing for 0328: settled, the camera turned 180 degrees
  in place; facing away the sun's bounce strength set to 2, settled, set back to 1, settled; turned
  back: the lit-side, shadow-foot and five even-ground pixels each within 1/255 of their values
  before the turn. Header paragraph for the case.
- `3d/src/src.md`, `3d/tests/tests.md`, `3d/include/3d/3d.md`: entries for the files above.

## Done when
`ctest --test-dir build/debug -R "^3d/(bounce|bounce_grid|bounce_scene|shadows)$"` passes.
