# 02 — A ray from a view's camera and the entity it meets
folder: 3d
decisions: 0168, 0177, 0191, 0202

## Change
The question "what is under this pixel", answered in `3d` and nowhere else. Card 01's store is what it is
answered from.

`3d/include/3d/pick.h` — new. Includes `<3d/shape_geometry.h>`, `<ecs/world.h>`, `<math/float3.h>`,
`<render/device.h>` (for `voe_platform_size`) and `<scene/camera_component.h>`, and declares:

- `voe_3d_ray` — `voe_math_float3 origin; voe_math_float3 direction;`, the direction unit length.
- `voe_3d_ray voe_3d_pick_ray(voe_scene_camera camera, voe_platform_size size, voe_math_float2 point)` —
  the ray through `point` of a picture `size` pixels big drawn with `camera`. `point` is in that picture's own
  pixels, x right and y down from its top-left corner, which is the shape every pointer in this engine already
  has (editor/src/main.c's division, ADR-0141 point 4). It builds `voe_3d_projection(camera, width / height)`
  and `voe_scene_camera_view(camera)` — the same two matrices the pass was opened with — inverts their product
  once with `voe_math_float4x4_inverse`, and takes the pixel's near point (clip z 1, because depth runs
  backwards) and its far point (clip z 0) through it, each divided by its own w: the origin is the near point
  and the direction is the normalised difference. Nothing in it writes the Y flip out by hand; the flip is in
  the one line that turns `point.y` into a clip coordinate, `1 - 2 * (point.y + 0.5f) / height`, the half pixel
  being the pixel's own centre. A size with no area, or a camera whose planes are the wrong way round, is the
  caller's bug and asserts, as `voe_3d_projection`'s already does.
- `voe_ecs_entity voe_3d_pick(const voe_ecs_world *world, const voe_3d_shape_geometries *geometries,
  voe_3d_ray ray, float *distance)` — the frontmost entity the ray meets, or a zeroed entity when it meets
  none. `distance` may be NULL; when it is not, it is how far along the ray the hit is, in metres, and is not
  touched when nothing was hit.

Its header says: why this is `3d`'s and not the editor's (it needs the shape table, the transform table and
the projection, and the editor is a leaf that reaches into no folder for a gap of its own — `editor/editor.md`);
that it walks the shape table alone and why that is the whole of what a project draws today (a mesh and a
material are runtime-only, `3d/mesh_component.h`, so no scene file holds one — 0202); how a hit is measured
(the entity's world matrix inverted takes the ray into the shape's own space, and the parameter along the ray
is the same number in both spaces because the direction is carried over without being normalised again); which
entities are skipped and why — no transform, a kind this build does not know, or a world matrix whose
determinant is nothing, which is a thing scaled away to nothing and drawn as nothing
(`scene/transform_component.h`); that ties are broken by table order and that this is not worth a rule; and
that it walks every row every call because a click is not a per-frame operation.

`3d/src/pick.c` — new. `voe_3d_pick` walks `voe_3d_shape_rows`/`voe_3d_shape_entities`, and for each row with
a transform and a known kind inverts `voe_scene_transform_matrix` once, carries the ray over with
`voe_math_float4x4_transform_point` and `_transform_dir`, and tests the kind's triangles with Möller–Trumbore,
keeping the smallest parameter above a small epsilon. The smallest over every entity is the answer. Write the
triangle test out once, in one static function, with its three lines of derivation in a comment.

`3d/tests/pick.c` — new, in two halves as `3d/tests/panel.c` already is.

The arithmetic half needs no graphics card. A world with transforms and shapes registered, one grey cube at the
origin, a camera at (0, 0, 5) looking down −Z with a 640×480 picture:
- the ray through the picture's centre hits the cube and the distance is 4.5 within 1e-3 (the cube is a metre
  across, ADR-0191);
- the ray through the top-left pixel meets nothing and the answer is a zeroed entity;
- a second cube at (0, 0, 2) makes the centre ray answer that one, whichever order the two rows are in — add
  them both ways round in two worlds;
- a cube moved to (2, 1, 0) is answered by a ray through the pixel its own centre projects to, worked out in
  the test with the same two matrices, and an entity with a shape but no transform is never answered.

The drawn half skips without a graphics card, exactly as `3d/tests/draw_system.c` does. A headless device, a
target, one shaped cube at (1.5, 1, 0) and a light, one frame drawn with `voe_3d_draw_system_run`, then
`voe_render_target_read` (ADR-0177): find a pixel that differs from the picture's top-left corner — that is a
pixel the cube covers — and check `voe_3d_pick` through it answers that cube; check that the top-left pixel
itself answers a zeroed entity. THIS IS THE CHECK THAT THE RAY AND THE PICTURE AGREE ABOUT WHICH WAY IS UP, and
its comment says so: the arithmetic half cannot catch a flipped Y because it works it out the same way.

`3d/3d.md`, `3d/src/src.md` and `3d/tests/tests.md` each gain their line; the tests one says the arithmetic
half needs no graphics card and the drawn half skips without one.

## Done when
`checks.sh 3d` exits 0 with `3d/pick` among the tests it ran, both halves running on this machine's card.
