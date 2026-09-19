# 06 — Capsule and cylinder shapes
folder: 3d
decisions: 0168, 0191, 0177

## Change
`include/3d/shape_component.h`: `#define VOE_3D_SHAPE_CAPSULE 2u` and `#define VOE_3D_SHAPE_CYLINDER 3u`. The
header no longer says the cube is the only kind.

New internal `src/capsule.h`/`.c` and `src/cylinder.h`/`.c`, beside `cube.h`/`.c`. Each has vertex and index
counts as constants and a builder,
`void voe_3d_capsule_build(voe_render_vertex *vertices, uint32_t *indices)` (and `_cylinder_`), that fills
caller arrays of those lengths. The shapes are 0191's, centred on the origin: 32 segments around, and each
capsule hemisphere 8 rings. Normals point outward at unit length, and the cylinder's caps have their own flat
normals. UVs run 0..1 around and along. Every triangle is counter-clockwise seen from outside, as `cube.c` is.

`include/3d/shape_system.h` / `src/shape_system.c`: `voe_3d_shapes` gains `voe_render_geometry capsule` and
`cylinder`. `voe_3d_shapes_upload` builds both into stack or file-scope arrays and creates their geometry
after the cube's. `VOE_3D_SHAPES_VERTICES` and `_INDICES` become the sums over all three shapes, written out
as numbers, with a `static_assert` in `shape_system.c` that each equals the internal constants' sum.
`_GEOMETRIES` becomes 3. The run gives each kind its own geometry. Update the header's capacity paragraph.
`editor/src/main.c` already sizes its device from these constants and needs no edit.

Update `src/src.md` and `tests/tests.md`.

## Done when
The folder's check passes (`checks.sh` for `3d`) with `tests/shape.c` extended. It includes `../src/capsule.h`
and `../src/cylinder.h`, as `ui/tests/theme.c` includes its own `src/`, and checks with no graphics card:
- every normal is unit length within 1e-4.
- every vertex lies within the shape's bounds: capsule |x|,|z| ≤ 0.5 and |y| ≤ 1, cylinder |x|,|z| ≤ 0.5 and
  |y| ≤ 0.5. Each bound is reached within 1e-4.
- every triangle's face normal points away from the Y axis, or away from the origin on a cap or hemisphere
  end.
- every index is below the vertex count.
On the GPU half, the run gives a capsule entity and a cylinder entity meshes on `shapes.capsule` and
`shapes.cylinder`.
