# 02 — A thing can name its parent, and its world place is composed
folder: scene
decisions: 0168, 0281, 0271, 0190, 0221

## Change
Needs card 01. The table, its reads and the world place (0281 points 1, 2, 5, 6). Parenting
itself is card 03. Follow the shape of `scene/include/scene/light_component.h` and
`light_system.h` (read their headers).

- New `scene/include/scene/parent_component.h`, `scene/src/parent_component.c`:
  - `voe_scene_parent` described with one field `F(voe_ecs_entity, parent, ENTITY)`;
    `voe_scene_parent_key` (name `voe_scene_parent`); `#define VOE_SCENE_PARENT_DEPTH_MAX 32`.
  - `const voe_scene_parent *voe_scene_parent_get(const voe_ecs_world *, voe_ecs_entity);` —
    NULL when the world never registered the table (find the type by walking the world's types
    for the key's address, as `transform_system.c` finds the identity), or the entity has no row.
  - `bool voe_scene_parent_within(const voe_ecs_world *, voe_ecs_entity entity,
    voe_ecs_entity ancestor);` — true when `ancestor` is `entity` or above it.
  - `uint32_t voe_scene_parent_tree(const voe_ecs_world *, voe_ecs_entity root,
    voe_ecs_entity *out, uint32_t capacity);` — `root`, then everything under it depth-first,
    children in the parent table's row order; returns how many were written, stopping at
    `capacity`. Explicit stack, no recursion.
  - Header points: no row is a root; why a row of its own and not a transform field; no replace
    and no menu, so it is shown and not edited and parented by `parent_system.h`; walks stop at
    the cap, a dead parent or one with no transform ends the chain (0281 point 6).
- New `scene/include/scene/parent_system.h`, `scene/src/parent_system.c`:
  `void voe_scene_parent_register(voe_ecs_world *, uint32_t capacity);` — table, description
  (or the compiled-out marker), default row the zeroed entity, needs the transform; no replace,
  no menu path. Header points: what registering gives, and that card 03's set lands here.
- `scene/include/scene/transform_component.h`, `scene/src/transform_component.c`:
  - `voe_scene_transform voe_scene_transform_world(const voe_ecs_world *, voe_ecs_entity);` —
    the row composed up the chain with `voe_scene_transform_compose`; the row itself with no
    parent table or no parent row. Asserts the entity has a transform.
  - `voe_scene_transform voe_scene_transform_local(const voe_ecs_world *, voe_ecs_entity,
    voe_scene_transform placed);` — the row that puts the entity at world `placed` under its
    current parent (`placed` for a root).
  - Rewrite the header's "THERE IS NO PARENT AND NO HIERARCHY" paragraph: the row is relative to
    its parent (0271), the world place is composed on demand and not stored, and which readers
    want `_world` versus the row.
- New `scene/tests/parent.c`: a hull, turret and barrel; `_world` of the barrel follows the hull
  moved and turned; a world without the table answers the row; `_within` both ways; `_tree` of
  the hull lists all three hull first; a two-entity loop written straight with
  `voe_ecs_component_add` ends `_world` without hanging; `_local` then `_world` round-trips.
  List it in `scene/tests/tests.md`.
- `scene/scene.md` (two entries) and `scene/src/src.md` (two entries).

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_scene $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_scene_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^scene/"` exits 0, `voe_test_scene_parent` among them.
