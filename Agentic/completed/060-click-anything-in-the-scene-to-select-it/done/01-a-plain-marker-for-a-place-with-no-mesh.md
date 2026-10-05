# 01 — A plain marker for a place with no mesh
folder: 3d
after: none
decisions: 0168, 0354, 0365

## Change
New public header and its source, modelled on the point light marker; read
`3d/include/3d/point_light_marker.h` and `3d/src/point_light_marker.c` and
copy their shape. Nothing else in the folder changes behaviour.

`3d/include/3d/place_marker.h` (new):
- `VOE_3D_PLACE_MARKER_EDGES` 12, `_VERTICES` (edges × 4), `_INDICES`
  (edges × 6).
- `bool voe_3d_place_marker_quads(voe_math_double3 position,
  voe_render_view view, voe_math_double3 eye, voe_platform_size size,
  float pixels, voe_base_arena *arena, voe_3d_outline_mesh *out)` — the
  octahedron's twelve edges (tips 0.25 m along ±X, ±Y, ±Z of the world
  axes) through `marker_lines.h`'s quads; false, `out` untouched, for a
  size with no area.
- `bool voe_3d_place_marker_hit(voe_math_double3 position, voe_3d_ray ray,
  float *distance)` — the world-axis cube of half extent 0.25 m, through
  `marker_lines.h`'s slab test, as the lamp's.
- `bool voe_3d_place_marker_wanted(const voe_ecs_world *world,
  voe_ecs_entity entity)` — true for a live entity with a transform and no
  row in any of: shape, model, water, mesh, panel (3d's tables), camera,
  light, point light (scene's tables). A table the world never registered
  counts as no row: check registration first, as `has_store` in
  `3d/src/pick.c` does (copy it as a static in the new source; registration
  is needed because `ecs` asserts on an unregistered key).
- Header points: why a diamond (a point with no direction, unlike the
  lamp's ball and sun's arrow), world axes and position only, a cube is hit,
  and that the draw and the pick both ask `_wanted` so what is drawn is
  what is clicked (0365 points 1–2).

`3d/src/place_marker.c` (new): the three functions; the edges are segments
handed to `marker_lines.h` as `point_light_marker.c` does.

`3d/tests/place_marker.c` (new; the build finds it):
- quads give `_VERTICES` vertices and `_INDICES` indices; no area is false;
- a ray down −Z through the position hits at the cube's face (distance
  checked), one 0.3 m to the side misses;
- `_wanted`: true for a bare transform, for a transform plus a light
  blocker row; false with no transform, with a shape, with a point light,
  with a camera; true in a world that registered no water, mesh or panel
  table (build the worlds as `3d/tests/point_light_marker.c` does).

Maps: `3d/3d.md` gains the header's entry (what it is, what its header
says) after the point light marker's; `3d/src/src.md` and
`3d/tests/tests.md` each gain their file's entry.

## Done when
`ctest --test-dir build/debug -R '^3d/place_marker$'` passes after a build.
