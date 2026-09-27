# 04 — The sun is a marker a ray picks
folder: 3d
decisions: 0168, 0274, 0223

## Change
Additive: a new header and one more thing `voe_3d_pick` can answer.

- New `3d/include/3d/sun_marker.h` and `3d/src/sun_marker.c`, shaped after
  `3d/include/3d/camera_marker.h` (read its header; reuse `camera_marker.c`'s quad width and
  winding the way `collider_marker.c` does, without copying it if a helper can be shared inside
  the folder):
  - `VOE_3D_SUN_MARKER_EDGES`, `_VERTICES`, `_INDICES`: a circle of 24 edges, the arrow's shaft
    and a head of four short edges.
  - `voe_3d_sun_marker_quads(voe_scene_transform pose, voe_render_view view,
    voe_math_double3 eye, voe_platform_size size, float pixels, voe_base_arena *arena,
    voe_3d_outline_mesh *out)` → bool: the circle of radius 0.25 m in the sun's XY plane and the
    arrow 1 m along its −Z, under position and rotation only (scale ignored, so a sun is never
    scaled out of sight), about `eye` in float (0250). False for a size with no area.
  - `voe_3d_sun_marker_hit(voe_scene_transform pose, voe_3d_ray ray, float *distance)` → bool:
    the slab test against a cube of half extent 0.25 m about the position in the sun's own
    axes, `distance` along the world ray as the camera's is.
  - The header says why lines, why scale is ignored and why a cube and not the lines is hit.
- `3d/src/pick.c`: after the camera boxes, walk the light table; each light with a transform is
  met by `voe_3d_sun_marker_hit`, nearest of all wins. `3d/include/3d/pick.h`: its paragraph on
  what is walked names the sun.
- New `3d/tests/sun_marker.c`: the edge counts in the mesh, a turned sun's arrow tip at 1 m
  along the turned −Z, a scaled sun building the same quads as an unscaled one, a ray hitting
  the cube square on, turned, and missing.
- `3d/tests/pick.c`: a ray through a sun in front of a cube picks the sun; behind it, the cube.
- `3d/3d.md` (new header), `3d/src/src.md`, `3d/tests/tests.md`: an entry each.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0, `3d/sun_marker` and `3d/pick` among them.
