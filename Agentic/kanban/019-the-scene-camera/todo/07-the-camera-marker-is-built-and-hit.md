# 07 — The camera marker is built and hit
folder: 3d
decisions: 0168, 0203, 0223

## Change
A new module in `3d`; nothing calls it until card 08.

- `3d/include/3d/camera_marker.h` (new) — header in the shape of `3d/include/3d/outline.h`, whose line-quad
  rules it reuses: fixed pixel width worked out per vertex from its own depth, quads extended half a width past
  each end, winding facing the eye, everything in the caller's arena. Points to make: the marker's geometry
  is 0223's (a box of half extents 0.1, 0.075, 0.15 m centred on the camera; a frustum from the camera's origin
  to 1 m ahead along −Z at the lens's fov_y and a 16:9 aspect), all in the camera's own space under its whole
  transform matrix, so it rolls and scales with it; only the box is hit.
  - `VOE_3D_CAMERA_MARKER_EDGES` (the box's 12 plus the frustum's 8), `_VERTICES` and `_INDICES` from it, as
    outline.h does.
  - `[[nodiscard]] bool voe_3d_camera_marker_quads(voe_scene_transform pose, voe_scene_camera lens,
    voe_render_view view, voe_platform_size size, float pixels, voe_base_arena *arena,
    voe_3d_outline_mesh *out)` — builds the marker's line quads as seen through `view`. False, with `out`
    untouched, when the pose's matrix has no inverse (a camera scaled to nothing) or `size` has no area.
  - `[[nodiscard]] bool voe_3d_camera_marker_hit(voe_scene_transform pose, voe_3d_ray ray, float *distance)`
    — the ray taken into the camera's space by the inverse of its matrix (the direction not renormalised, so
    the slab test's parameter stays a distance along the world ray) and tested against the box. False for no
    hit or a pose with no inverse (checked by `voe_math_float4x4_determinant` before inverting); `distance`
    may be NULL.
- `3d/src/camera_marker.c` (new) — both. The per-vertex width and the quad for one edge are outline.c's
  arithmetic: reuse any helper outline.c already exposes; if it has none, write them here and say in the
  header that they mirror outline.c's.
- `3d/tests/camera_marker.c` (new), registered the way the folder's other tests are
  (`3d/tests/tests.md` says how) as `camera_marker`:
  - an identity pose seen from (0, 0, 5) builds 20 edges' worth of vertices and indices;
  - a pose with scale zero builds nothing and is not hit;
  - a ray from (0, 0, 5) down −Z hits an identity pose at 4.85 m;
  - the same ray against a pose at (0, 0, 0) turned 90° about Y hits at 4.9 m (the box's x half-extent);
  - a ray from (1, 0, 5) down −Z misses.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — one entry each for the new file.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `3d` exits 0 and
`ctest --test-dir build/debug -R "^3d/camera_marker"` runs at least one test and passes.
