# 02 — 3d gives an entity tree's size and the distance that frames it
folder: 3d
after: none
decisions: 0168, 0371

## Change
A new module answering how big an entity and its children are in the world, and how far an eye
stands to frame that size (decision 0371 points 1 and 2). No new folder edge: `3d` already
depends on `scene`.

- `3d/include/3d/bounds.h` (new) — header comment points: why this is `3d`'s question (the shape
  table, the model store and the transforms are here, as pick.h says for the ray); what counts as
  size (shapes and loaded models only; water, particles, markers, colliders none); the box is
  axis-aligned in the world and given as a sphere so the answer does not depend on a view's angle;
  ADR-0250: the box is measured in float about a double origin and the centre handed back in double.
  Two functions:
  - `bool voe_3d_bounds(const voe_ecs_world *world, const voe_3d_shape_geometries *geometries,
    const voe_3d_models *models, voe_ecs_entity root, voe_math_double3 *centre, float *radius)` —
    walks every shape row and every model row whose entity is `root` or under it
    (`voe_scene_parent_within`, scene/parent_component.h) and has a transform; each one's vertices
    go through `voe_scene_transform_matrix(voe_scene_transform_world(...), origin)`, the origin
    the first such entity's world position; min and max kept in float. `centre` is origin plus the
    box's middle, `radius` half its diagonal. False, outputs untouched, when nothing counted. A
    NULL `models`, a model with no loaded entry, a kind with no geometry, and a world that never
    registered a table walk none of it, as pick.c does.
  - `float voe_3d_bounds_distance(float radius, float fov_y, float aspect, float fill)` — the eye's
    distance from the centre at which the sphere spans `fill` of the narrower of the picture's
    height and width: `h` = min(fov_y / 2, atan(aspect · tan(fov_y / 2))), distance
    `radius / sin(atan(fill · tan h))`.
- `3d/src/bounds.c` (new) — the two. Read the header of `3d/include/3d/pick.h` for the tables
  and the model store, `3d/include/3d/shape_geometry.h` for a kind's vertices,
  `3d/include/3d/models.h` for an entry's `shape`, `scene/include/scene/transform_component.h`.
- `3d/tests/bounds.c` (new) — plain C program as the folder's others; set the world up the way
  `3d/tests/pick.c` does. Checks: a built-in cube scaled 2 at (10, 0, 0) gives that centre and a
  radius of the scaled cube's half diagonal; a parent with no shape and a cube child under it gives
  the child's box, and a cube on the parent plus a far child gives the box round both; an entity
  with only a transform answers false; the same cube 100 km out keeps its centre to a millimetre;
  `voe_3d_bounds_distance(1, π/2, 1, 1)` is √2, and an aspect under 1 gives a larger distance than
  aspect 1.
- `3d/3d.md` — an entry for `include/3d/bounds.h`. `3d/src/src.md`, `3d/tests/tests.md` — an
  entry each for the new files.

## Done when
`ctest --test-dir build/debug -R "^3d/bounds"` passes after a debug build of `voe_test_3d_bounds`.
