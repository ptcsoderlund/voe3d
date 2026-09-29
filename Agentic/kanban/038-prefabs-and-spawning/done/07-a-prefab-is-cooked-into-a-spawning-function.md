# 07 — A prefab is cooked into a function that spawns it
folder: authoring
decisions: 0168, 0283, 0237

## Change
Needs card 06. 0283 point 9: the C a game spawns a prefab with.

- `authoring/include/authoring/prefab.h`: `[[nodiscard]] bool voe_authoring_prefab_cook(
  const voe_ecs_world *world, const char *function, voe_base_arena *arena,
  voe_authoring_text *out, uint32_t *out_entities);`. `world` holds one prefab, read into it by
  `voe_authoring_scene_read`. The text is one definition, no includes:
  `static bool <function>(voe_ecs_world *world, const voe_ecs_entity *entities,
  voe_math_double3 position, voe_math_quat rotation)`. Entity order: the root (the one entity
  with no parent row) is `entities[0]`, the rest follow in ascending id. For each entity, every
  described, not runtime-only type except `voe_scene_identity` is queued with
  `voe_ecs_structure_add(world, voe_ecs_component_type(world, &<key>), entities[i], &(<struct>){…})`,
  returning false when one is refused; the root's `voe_scene_transform` is `position`,
  `rotation` and scale one instead of its row; an ENTITY field naming an entity of the prefab is
  `entities[j]`, any other a zeroed entity. `*out_entities` is the entity count. Refuses as
  scene_cook does, and a world without exactly one root. Header points: why structure adds and
  not component adds (a spawn lands mid-step, at the step's structural apply); why no identity
  (spawned things are not authored, and the identity table stays 32); the caller makes the
  entities and owns the include line.
- `authoring/src/prefab_cook.c` (new): reuse `authoring/src/scene_cook.c`'s spelling of a
  field's value through a private header both use, not a copy; `scene_cook.c`'s output stays
  byte for byte.
- `authoring/tests/prefab.c`: a hull with a shape and a turret under it, cooked as
  `enemy_tank`: the text defines `static bool enemy_tank(`, holds two `voe_ecs_structure_add`
  groups for `entities[0]` and `entities[1]`, the turret's parent is `entities[0]`, the root's
  transform uses `position` and `rotation`, and no `voe_scene_identity` appears;
  `*out_entities` is 2. Two roots are refused.
- `authoring/src/src.md`, `authoring/tests/tests.md`: entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_authoring $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_authoring_[A-Za-z0-9_]+") && ctest --test-dir build/debug
-R "^authoring/"` exits 0, `voe_test_authoring_prefab` and `voe_test_authoring_scene_cook`
among them.
