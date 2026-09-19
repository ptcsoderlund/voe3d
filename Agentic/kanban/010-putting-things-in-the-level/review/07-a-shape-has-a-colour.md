# 07 — A shape has a colour
folder: 3d
decisions: 0168, 0190, 0191, 0193, 0177

## Change
`include/3d/shape_component.h` / `src/shape_component.c`:
- Fields: `F_READ_ONLY(uint32_t, kind, UINT32)` then `F(voe_math_float3, colour, COLOUR)`.
  `#define VOE_3D_SHAPE_GREY ((voe_math_float3){ 0.7f, 0.7f, 0.7f })`.
- `voe_3d_shape_register` sets the default row `{ VOE_3D_SHAPE_CUBE, VOE_3D_SHAPE_GREY }` and
  `voe_ecs_component_needs_set(world, shape, <transform's type>)`. The transform type is found through
  `voe_ecs_component_type(world, &voe_scene_transform_key)`, so transform must be registered first; say so.
- The header: kind is read-only, colour is edited through the intent below and is linear (0191).

`include/3d/shape_system.h` / `src/shape_system.c`:
- `typedef struct { voe_ecs_entity entity; voe_3d_shape shape; } voe_3d_shape_intent;`,
  `[[nodiscard]] bool voe_3d_shape_submit(voe_ecs_world *, voe_3d_shape_intent)`. The intent queue is
  registered in `voe_3d_shape_register` and set as the shape's replace (ecs/component.h), capacity = the table's.
- `voe_3d_shape_system_run` drains it first. An intent for a dead entity or one with no shape is dropped. `kind`
  is put back to the entity's own, and each colour channel is clamped to 0..1. Both corrections are reported the
  way the unknown-kind warning is, edge-triggered.
- The shapes' one material is uploaded white (1, 1, 1), not grey.
- After giving meshes, the run finds each entity whose mesh is on one of `shapes`' geometries and whose material's
  `shading` is `shapes.material.shading`, and that has no shape. It submits `voe_ecs_structure_remove` for its
  mesh and its material (0190). A submit the queue refuses asserts: the program chose the world's capacities.
  The header says the world needs a structural queue, and why a removed shape draws one more frame.

`src/draw_system.c`: `object_of` sets `colour` to the entity's shape colour with alpha 1, or (1, 1, 1, 1) when
it has no shape. `include/3d/draw_system.h` says where a drawn object's colour comes from.

Tests that make a world with shapes (`tests/shape.c`, and any other that registers one) register transform
before shape and give the world `structure_requests`/`structure_bytes`.

Downstream (ADR-0113): `editor/src/project.c`'s untitled cube gets `.colour = VOE_3D_SHAPE_GREY`.
`editor/src/main.c`'s comment above `voe_3d_shape_system_run` ("NOT AN INTENT DRAIN") now says the run drains
the shape's intent.

Update `3d.md`, `src/src.md` and `tests/tests.md`.

## Done when
The folder's check passes (`checks.sh` for `3d`) with:
- `tests/shape.c`: the default row and the needs are as above. An intent that changes kind lands with kind put
  back and colour applied, and a colour of (2, -1, 0.5) lands as (1, 0, 0.5). Removing a shape, then
  `voe_ecs_structure_apply`, run, apply, leaves the entity with no mesh and no material. An imported-style entity
  (a mesh on other geometry, no shape) keeps its mesh.
- `tests/draw_system.c`: a shaped cube coloured (1, 0, 0) in front of the camera reads red at the picture's
  centre (skips without a graphics card).
With `C=$(mktemp -d)`, `./build/debug/editor/voe_editor --capture $C/o.png` exits 0, and reading it (ADR-0177)
shows the untitled cube grey, as before.
