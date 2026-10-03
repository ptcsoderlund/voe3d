# 37 — A mesh's own box, handed back by render
folder: render
after: none
decisions: 0168, 0332

## Change
0332 point 1, for bug 03 of 051: 3d will fit the bounce grid to the box of the still casters,
and only render has a mesh's vertices. Render keeps a bounding sphere per mesh already; a sphere
of a flat 40 m ground is a 70 m cube, so it keeps a box too. Nothing calls the query yet.
- `render/src/device_parts.h`: `struct voe_render_geometry_slot` gains the mesh's own vertex box,
  min and max corners (`voe_math_float3`), beside `sphere`; its comment says what it bounds and
  who reads it (the query below).
- `render/src/geometry.c`: both creates (static and transient) set the box from the vertices where
  they set the sphere, in the same walk.
- `render/include/render/device.h`, in the geometry section after `voe_render_geometry_destroy`:
  `bool voe_render_geometry_box(const voe_render_device *device, voe_render_geometry geometry,
  voe_math_float3 *min, voe_math_float3 *max)`. Comment: the mesh's own vertices' box in its own
  space, taken once at create; false and nothing written for an id that names nothing (destroyed,
  stale transient); no frame needed, no GPU touched; why it exists (a caller fitting something to
  where meshes stand, 0332).
- `render/tests/pools.c`: a case: a mesh whose vertices span (−1, 0, −2) to (3, 5, 2) hands back
  exactly that box; after its destroy the query is false. Header paragraph for the case.
- `render/include/render/render.md` (if it lists device.h's sections), `render/src/src.md`
  (`geometry.c`), `render/tests/tests.md` (`pools.c`): entries.

## Done when
`ctest --test-dir build/debug -R "^render/pools$"` passes.
