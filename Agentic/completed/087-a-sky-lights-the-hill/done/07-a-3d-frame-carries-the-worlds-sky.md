# 07 — A 3d frame carries the world's sky
folder: 3d
after: 01, 04
decisions: 0168, 0405

## Change
The world's sky row into the pass camera (0405 point 2), so the game, the editor's views and its preview
draw it with no code of their own. Read the headers of `3d/include/3d/draw_system.h` (`voe_3d_frame`,
`voe_3d_draw_system_frame`, `voe_3d_draw_system_camera`), `3d/src/draw_system.c`,
`scene/include/scene/sky_component.h` (card 01), `voe_render_sky` in `render/include/render/device.h`,
and `3d/tests/no_light.c` and `3d/tests/light_blockers.c` as patterns.

- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `voe_render_sky sky` after `blockers`, commented
  as its neighbours: `_frame` fills it from the world's first sky row (drawn 1, light, haze, exposure,
  tone copied, the tone's 0 and 1 meaning render's), zeroed with no sky table or no row; the
  `_frame` comment names it among what it works out; `voe_3d_draw_system_camera`'s comment names it.
- `3d/src/draw_system.c`: `_frame` fills it; `_camera` passes it as `.sky`. Header names the sky.
- `3d/tests/sky.c` (new): a world with transforms, a camera and a light, no sky table: `sky` zeroed;
  the table with no row: zeroed; a row with haze 500, exposure −1, tone Clip: drawn 1 and the same
  numbers, and `voe_3d_draw_system_camera` carries them. A GPU case modelled on
  `light_blockers.c`'s: sun overhead, a sky row, one shape in the middle of the target, drawn through
  `_frame`, `_camera` and `_run`; a corner pixel reads blue above red and not the clear colour, the
  shape's centre reads its lit colour; without the row the corner reads the clear colour.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the draw_system entries name the sky; one new for
  the test. Each at most 300 characters.

## Done when
The tests `3d/sky`, `3d/no_light` and `3d/draw_system` pass after the folder's build.
