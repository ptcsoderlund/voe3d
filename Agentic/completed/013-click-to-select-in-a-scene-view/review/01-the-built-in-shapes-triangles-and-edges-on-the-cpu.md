# 01 — The built-in shapes' triangles and edges on the CPU
folder: 3d
decisions: 0168, 0191, 0202, 0203

## Change
A new module that keeps, on the CPU, the geometry the three built-in shapes are already built from, plus the
edges of each surface — what a ray is cast against (card 02) and what a silhouette is walked over (card 03).
Nothing here talks to a device.

`3d/include/3d/shape_geometry.h` — new. It includes `<base/arena.h>`, `<math/float3.h>` and
`<render/device.h>` (for `voe_render_vertex`), and declares:

- `voe_3d_shape_edge` — one edge of a shape's surface, in the shape's own space: `voe_math_float3 a, b` for
  where it runs and `voe_math_float3 left, right` for the outward normal of each of the two triangles that meet
  along it. An edge with only one triangle carries the same normal twice.
- `voe_3d_shape_geometry` — `const voe_render_vertex *vertices; uint32_t vertex_count; const uint32_t *indices;
  uint32_t index_count; const voe_3d_shape_edge *edges; uint32_t edge_count;`.
- `voe_3d_shape_geometries` — `voe_3d_shape_geometry kinds[4];`, indexed by the kind's own number, entry nought
  zeroed, exactly as `voe_3d_shape_kind_names` is indexed (3d/shape_component.h).
- `void voe_3d_shape_geometries_create(voe_base_arena *arena, voe_3d_shape_geometries *out)` — a startup
  operation with no device in it; everything it hands back lives in `arena` and is read-only afterwards.
- `const voe_3d_shape_geometry *voe_3d_shape_geometry_of(const voe_3d_shape_geometries *geometries,
  uint32_t kind)` — NULL for nought and for a kind this build does not know, which is the same answer
  `voe_3d_shape_kind_names` gives such a kind.

Its header says: why the CPU keeps a copy of geometry the card already has (a ray cannot be cast at a
device-local buffer, and a silhouette is a walk over edges rather than over pixels — 0202, 0203); why it is
built rather than stored as data (`src/cube.h`, `src/capsule.h` and `src/cylinder.h` already produce it, and a
second hand-written copy is a second truth); why the weld is by position and not by index (a cube's corner is
three vertices and a capsule's seam two, because a vertex carries one normal and one texture coordinate —
`src/cube.h` says the same thing) and what the tolerance is; that a closed surface leaves every edge with two
triangles and a boundary edge is not a fault; why it is keyed by kind (a kind is what a saved scene holds,
ADR-0191); and that the arena is the caller's and must outlive the answer (rule 11).

`3d/src/shape_geometry.c` — new. `voe_3d_shape_geometries_create` fills the three entries:

- The cube points straight at `voe_3d_cube_vertices` and `voe_3d_cube_indices`; nothing is copied for it.
- The capsule and the cylinder get arrays in `arena` exactly their two constants long and are built into them
  with `voe_3d_capsule_build` and `voe_3d_cylinder_build`.
- The edges of each: first weld the vertices by position — a representative number per vertex, two positions
  the same when every component differs by less than `VOE_3D_SHAPE_GEOMETRY_WELD` (1e-4 metres, a tenth of a
  millimetre on shapes a metre across; a `#define` in the header beside the struct) — then walk the triangles.
  Each triangle's outward normal is `normalize(cross(b - a, c - a))`, which points outward because every
  built-in shape is wound counter-clockwise seen from outside (`src/cube.h`); each of its three edges is keyed
  by its two welded numbers, smaller first, in an open-addressed table the size of the next power of two above
  three times the triangle count, and either starts an edge — `left` set, `right` the same — or fills the
  `right` of the one already there. The welded vertices' positions are the edge's `a` and `b`.
  The table and the per-vertex numbers are scratch: take an arena mark before them and rewind to it after the
  edges are copied down to their final count, so what survives is the three arrays and nothing else.

`3d/tests/shape_geometry.c` — new. Needs no graphics card. One arena, one `_create`, then:

- kind nought and kind 99 answer NULL; the three kinds answer their own entry.
- The cube's counts are `VOE_3D_CUBE_VERTICES` and `VOE_3D_CUBE_INDICES` and its `vertices` pointer is
  `voe_3d_cube_vertices` itself; the capsule's and the cylinder's are their own two constants.
- Every kind is a closed surface: `edge_count` is exactly `index_count / 2` — three edges a triangle, two
  triangles an edge. For the cube every edge's two normals differ (a cube edge is a fold), and for all three
  kinds every normal an edge carries is unit length within 1e-4.
- Every triangle's computed outward normal agrees with the normals its own three vertices carry: the dot
  product is above nought for all of them. That is what proves the winding this file assumes.

`3d/3d.md` gains a line for `include/3d/shape_geometry.h`; `3d/src/src.md` one for `shape_geometry.c`;
`3d/tests/tests.md` one for `shape_geometry.c`, saying it needs no graphics card.

## Done when
`checks.sh 3d` exits 0 with `3d/shape_geometry` among the tests it ran and passing on this machine.
