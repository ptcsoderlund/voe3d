# 05 — A gizmo has rings to turn by
folder: 3d
decisions: 0168, 0274, 0205, 0207, 0250

## Change
Additive: a new header beside `3d/include/3d/gizmo.h` (read its header first; it is the model).

- New `3d/include/3d/gizmo_rings.h`, `3d/src/gizmo_rings.c`, all over `voe_3d_gizmo` (from
  `voe_3d_gizmo_at`) and `voe_3d_gizmo_handle`'s X, Y and Z, each naming the ring about that
  world axis, radius one `shaft`:
  - `VOE_3D_GIZMO_RING_SEGMENTS` 48, `VOE_3D_GIZMO_RING_VERTICES`, `_INDICES`: three rings of
    camera-facing quads, `VOE_3D_GIZMO_LINE_HALF_WIDTH` wide.
  - `voe_3d_gizmo_rings_hit(voe_3d_gizmo, voe_3d_ray)` → handle: each ring's plane crossed in
    front of the eye within `VOE_3D_GIZMO_GRIP` of the radius, nearest along the ray first;
    NONE for none, a ray in the plane, a gizmo behind the eye or of no shaft.
  - `voe_3d_gizmo_rings_angle(voe_3d_gizmo, voe_3d_gizmo_handle, voe_3d_ray, float *out)` →
    [[nodiscard]] bool: the angle about the axis, right-handed, of where the ray crosses the
    ring's plane, measured about the origin; false with `*out` untouched for NONE, a plane
    handle, or a ray too nearly in the plane.
  - `voe_3d_gizmo_rings_quads(voe_3d_gizmo, voe_3d_gizmo_handle marked, voe_base_arena *,
    voe_3d_gizmo_mesh *plain, voe_3d_gizmo_mesh *marked_out)` → [[nodiscard]] bool: as
    `voe_3d_gizmo_quads`, the marked ring at `VOE_3D_GIZMO_MARKED_STEP` width, about the eye.
  - The header says why world axes (0205's reason), why the angle is measured and not a delta,
    and why a ring is quads facing the eye.
  - Reuse `gizmo.c`'s quad helpers by moving them into an internal `3d/src/gizmo_quads.h/.c`
    if both files need them; keep `gizmo.c` under 800 lines.
- New `3d/tests/gizmo_rings.c`: a ray through each ring's rim picks it, one through the centre
  picks none, the nearer of two crossed rings wins, a quarter turn of the pointer about Y
  reads π/2 within 1e-4, a ray in the plane refuses, 100 km out the same, the two meshes'
  counts and winding towards the eye, and the marked ring moving from plain to marked.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: an entry each.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0, `3d/gizmo_rings` among them.
