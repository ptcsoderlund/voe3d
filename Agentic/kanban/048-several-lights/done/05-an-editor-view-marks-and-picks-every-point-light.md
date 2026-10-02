# 05 — An editor view marks and picks every point light
folder: 3d
after: 04
decisions: 0168, 0274, 0320

## Change
A point light's marker and its pick (0320 point 7), built as the sun's. Read
`3d/include/3d/sun_marker.h`, `3d/src/sun_marker.c`, `3d/src/marker_lines.h`, `3d/src/draw_marks.h`,
`3d/src/draw_marks.c`'s `voe_3d_draw_marks_sun`, `3d/src/pick.c`'s sun walk, `3d/include/3d/pick.h`'s
header, and `voe_3d_sun_marked` and the `sun` member in `3d/include/3d/draw_system.h`.

- `3d/include/3d/point_light_marker.h` (new), shaped as sun_marker.h:
  - `VOE_3D_POINT_LIGHT_MARKER_CIRCLE` 12 (edges a circle), `_EDGES` (3 × circle), `_VERTICES`
    (edges × 4), `_INDICES` (edges × 6).
  - `[[nodiscard]] bool voe_3d_point_light_marker_quads(voe_math_double3 position, voe_render_view
    view, voe_math_double3 eye, voe_platform_size size, float pixels, voe_base_arena *arena,
    voe_3d_outline_mesh *out)`: three circles of radius 0.25 m about `position`, in the world's XY, YZ
    and ZX planes, through `voe_3d_marker_lines`; false with `out` untouched when `size` has no area.
  - `[[nodiscard]] bool voe_3d_point_light_marker_hit(voe_math_double3 position, voe_3d_ray ray,
    float *distance)`: the world-axis cube of half extent 0.25 m about `position`, as the sun's hit.
  - Header points: a wire ball because a lamp has a place and no direction; world axes since rotation
    and scale change nothing it lights; a cube picks, not the lines.
- `3d/src/point_light_marker.c` (new): the two calls.
- `3d/include/3d/draw_system.h`: `voe_3d_point_lights_marked { bool shown; voe_ecs_entity selected;
  voe_3d_material material; voe_math_float3 colour; voe_math_float3 selected_colour; float pixels;
  voe_platform_size size; }` beside `voe_3d_sun_marked`, and `voe_3d_frame` gains `point_lights` of
  it after `sun`. Comment points: only an editor's view sets it; every point light with a transform is
  marked when `shown`, the `selected` one in `selected_colour`, the rest in `colour`; as two transient
  geometries (the rest together, the selected alone), so the pool needs VOE_3D_POINT_LIGHT_MARKER_
  VERTICES and _INDICES per light and two ranges and two objects; a pool too small draws none, as
  the outline. `voe_3d_draw_system_frame` zeroes it; its comment's list names it.
- `3d/src/draw_marks.h`, `3d/src/draw_marks.c`: `void voe_3d_draw_marks_point_lights(const
  voe_ecs_world *world, voe_render_device *device, voe_base_arena *arena, voe_3d_frame frame)`, as the
  sun's, at the same lag the sun's marker uses; nothing with no point light table.
- `3d/src/draw_system.c`: `_run` calls it right after `voe_3d_draw_marks_sun`.
- `3d/src/pick.c`: after the suns, the point lights on their cubes, under `has_store` of the point
  light key, at the world place. `3d/include/3d/pick.h` header: point lights in the list of what is
  picked.
- `3d/tests/point_light_marker.c` (new), modelled on `3d/tests/sun_marker.c`: the quads' count is
  _EDGES × 4 vertices seen from off-axis; no area is false; the hit meets the cube head-on at
  0.25 m short of the position and misses beside it; `voe_3d_pick` returns a point light under the
  ray and the nearer of a light and a shape.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: one entry each for the new files; the pick.h and
  draw_system.h entries name point lights. Each at most 300 characters.

## Done when
The tests `3d/point_light_marker`, `3d/sun_marker`, `3d/pick` and `3d/point_lights` pass after the
folder's build.
