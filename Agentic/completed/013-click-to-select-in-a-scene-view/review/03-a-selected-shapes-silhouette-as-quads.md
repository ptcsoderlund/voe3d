# 03 — A selected shape's silhouette as quads
folder: 3d
decisions: 0168, 0191, 0203

## Change
The outline itself, as geometry: one entity's silhouette, seen from one camera, as quads standing in the world.
It builds arrays and uploads nothing; card 05 is what draws them.

`3d/include/3d/outline.h` — new. Includes `<3d/material_component.h>`, `<3d/shape_geometry.h>`,
`<base/arena.h>`, `<ecs/world.h>` and `<render/device.h>`, and declares:

- `VOE_3D_OUTLINE_EDGES` 512, the most edges one outline is built from, and `VOE_3D_OUTLINE_VERTICES`
  (`VOE_3D_OUTLINE_EDGES * 4`) and `VOE_3D_OUTLINE_INDICES` (`VOE_3D_OUTLINE_EDGES * 6`), which is what a
  program sizes `voe_render_capacities`' three transient numbers from.
- `voe_3d_outlined` — what a pass outlines and how: `voe_ecs_entity entity` (a zeroed one outlines nothing),
  `const voe_3d_shape_geometries *geometries` (card 01's store, NULL for none), `voe_3d_material material`
  (the unlit record the quads wear — card 04's `voe_3d_shapes.outline`), `voe_math_float3 colour` (linear),
  `float pixels` (how wide the line is on the picture) and `voe_platform_size size` (that picture's size).
- `voe_3d_outline_mesh` — `const voe_render_vertex *vertices; uint32_t vertex_count; const uint32_t *indices;
  uint32_t index_count;`, all of it in the arena the build was handed.
- `[[nodiscard]] bool voe_3d_outline_quads(const voe_ecs_world *world, voe_3d_outlined outlined,
  voe_render_view view, voe_base_arena *arena, voe_3d_outline_mesh *out)` — false, with nothing written, when
  there is nothing to outline: a zeroed or dead entity, no store, an entity with no shape or no transform, a
  kind this build does not know, or a silhouette with no edges at all.

Its header says: what a silhouette edge is and why the set of them is exactly the outline the drawn triangles
have (an edge whose two triangles disagree about which side of them the eye is on); why the quads stand in the
world rather than on the screen (they are drawn through the same camera as everything else, so there is no
second space and no orthographic anything — `3d/draw_system.h` says the same about the overlay layer); why the
width is worked out per vertex from its own depth, which is what keeps the line the same thickness far away as
near; why the quads are extended half a width past each end (a corner between two of them would otherwise show
a notch); why they are wound to face the eye (the pipeline culls back faces) and carry normals pointing at it
even though the record they wear is unlit; why `VOE_3D_OUTLINE_EDGES` is a cap and not an assert (a shape
nobody has measured must not fill the frame's transient pool — the outline is missing a few edges rather than
the frame being refused); and that everything it hands back is the caller's arena's and dies with it.

`3d/src/outline.c` — new:

- Read the shape (`voe_3d_shape_get`), the transform (`voe_scene_transform_get`) and the kind's geometry
  (`voe_3d_shape_geometry_of`). Build the world matrix (`voe_scene_transform_matrix`) and its inverse, and take
  the eye — `view.eye` — into the shape's own space with the inverse.
- Walk the kind's edges. An edge is a silhouette edge when `dot(left, eye_local - a)` and
  `dot(right, eye_local - a)` have different signs; an edge whose two normals are the same is one too.
- For each of them, in world space: `a` and `b` through the world matrix; for each end, its depth in front of
  the eye `d = -(view.view.m[2][0] * p.x + view.view.m[2][1] * p.y + view.view.m[2][2] * p.z +
  view.view.m[2][3])`, and a half width `d * outlined.pixels / (view.projection.m[1][1] * size.height)` — the
  projection's second diagonal is one over the tangent of half the vertical field of view, so that is exactly
  `pixels` pixels across whatever the distance. Along the edge, `e = normalize(b - a)`; away from the eye,
  `v = normalize(a_world - view.eye)`; outward, `out = normalize(cross(e, v))`, negated when
  `dot(out, n_front)` is below nought, `n_front` being whichever of the edge's two normals the eye is in front
  of, carried into world space by the world matrix's transform of a direction.
- Emit four vertices — `a - e * w`, `b + e * w`, and those two moved out by `out * 2 * w` — with normals
  pointing at the eye and zeroed texture coordinates, and two triangles, ordered so that
  `cross(second - first, third - first)` points at the eye; swap the two of each triangle when it does not.
- Stop at `VOE_3D_OUTLINE_EDGES`. Allocate the two arrays from `arena` at the cap's size once, fill them, and
  hand back the counts actually used.

`3d/tests/outline.c` — new. Needs no graphics card. A world with transforms and shapes registered, card 01's
store, and a `voe_render_view` built the way card 02's test builds its matrices:

- A cube at the origin seen from (0, 0, 5) has four silhouette edges — 16 vertices and 24 indices — and every
  vertex is within 0.71 metres of the origin in x and y (just outside the face, never inside the cube's own
  half metre).
- The same cube turned 45 degrees about Y has six.
- The same cube at twice the distance gives quads twice as wide in metres (measure one quad's two ends), which
  is what "the same width on the screen" means here.
- Every triangle faces the eye: its normal, from its own three positions, has a positive dot with the direction
  from the triangle to the eye.
- A capsule gives more than four edges and no more than `VOE_3D_OUTLINE_EDGES`.
- A zeroed entity, an entity with no shape, one with no transform and a shape of kind 99 each answer false and
  leave `out` untouched.

`3d/3d.md`, `3d/src/src.md` and `3d/tests/tests.md` each gain their line.

## Done when
`checks.sh 3d` exits 0 with `3d/outline` among the tests it ran and passing on this machine.
