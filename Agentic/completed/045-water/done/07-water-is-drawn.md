# 07 — Water is drawn
folder: 3d
after: 06
decisions: 0168, 0305

## Change
0305 point 7. Read `3d/include/3d/draw_system.h`, `3d/src/draw_system.c`,
`3d/src/draw_group.h`, `3d/src/draw_particles.h`,
`3d/src/draw_particles.c` (a blended world draw from the store),
`3d/include/3d/water_component.h`, `3d/include/3d/models.h`, and
`3d/tests/draw_particles.c` for a headless draw test.

- `3d/src/draw_water.h` and `3d/src/draw_water.c`, new, internal: the count
  for the group's room, and the walk over water rows with a transform and a
  waves row, each one object into the world blended group: world = the
  entity's eye-relative world matrix × the quad turned from XY to face +Y ×
  scale(width, length, 1); colour the row's colour; `waves` from the row
  and its clock; `sky` the row's sky. Nothing when the store has no water
  record. Header points: no shadow cast, why the turn, eye-relative.
- `3d/src/draw_system.c`: the water's room counted; when any water was
  held, `voe_render_frame_copy_depth` once after the world's solid draws and
  before its blended group; a false return is the run's failure as any draw.
- `3d/include/3d/draw_system.h`: header points for the copy and water.
- `3d/tests/draw_water.c`, new, headless: a ground cube below a 4 × 4 water
  under a sun; the store's water loaded; cases: the picture's centre
  differs from the same frame with no water; two frames with clocks 0 and
  1.3 differ; a store without the record draws no water and fails nothing;
  the shadow pass's draw count is the same with and without water.
- `3d/src/src.md`, `3d/tests/tests.md`: entries added.

## Done when
The test `3d/draw_water` passes, and `3d/draw_system` and
`3d/draw_particles` still pass, after the folder's build.
