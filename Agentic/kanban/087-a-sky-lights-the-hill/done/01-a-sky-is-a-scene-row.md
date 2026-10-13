# 01 — A sky is a scene row
folder: scene
after: none
decisions: 0168, 0405

## Change
The row of 0405 point 1, built as the light blocker's. Read `scene/include/scene/light_blocker_component.h`,
`scene/include/scene/light_blocker_system.h`, `scene/src/light_blocker_component.c`,
`scene/src/light_blocker_system.c`, `scene/tests/light_blocker.c` as the pattern, and the named `bounces`
field of `scene/include/scene/light_component.h` and `scene/src/light_component.c` for a named UINT32.

- `scene/include/scene/sky_component.h` (new): `voe_scene_sky` as a described field list, `light`,
  `haze`, `exposure` FLOAT32 and `tone` UINT32 named by `voe_scene_sky_tone_names` ("Clip", "Soft");
  `VOE_SCENE_SKY_TONE_CLIP` 0, `VOE_SCENE_SKY_TONE_SOFT` 1, `VOE_SCENE_SKY_EXPOSURE_MAX` 16; its key;
  `_get`, `_count`, `_rows`, `_entities`. Header points: the row is the sky on and no row the old
  picture; the sun it follows is the world's first directional light; what each field means, its unit
  and bounds (0405 point 1); the first row in table order is the one drawn.
- `scene/include/scene/sky_system.h` (new):
  - `void voe_scene_sky_register(voe_ecs_world *world, uint32_t capacity)`: the table, default row
    light 1, haze 1000, exposure 0, tone Soft, no needed type, menu "Rendering / Sky", the whole-row
    intent as its replace.
  - `[[nodiscard]] bool voe_scene_sky_add(world, entity, voe_scene_sky sky)`: direct; asserts on a row
    the drain would refuse; false when full or the entity is dead.
  - `voe_scene_sky_intent { entity; sky; }`, `[[nodiscard]] bool voe_scene_sky_submit(world, intent)`.
  - `void voe_scene_sky_system_run(voe_ecs_world *world)`: applies replaces in order, refusing (row
    kept, one stderr line) a number not finite, light or haze below nought, exposure past ±16, tone
    past Soft.
  - Header points: what is refused; why the replace is queued (rule 4).
- `scene/src/sky_component.c`, `scene/src/sky_system.c` (new): the above, each with its header comment.
- `scene/tests/sky.c` (new): registration (menu path, no needed type, default row); add and read back;
  a replace changes the haze after a run; a negative light, an infinite haze, a NaN exposure, an
  exposure of 17 and a tone of 2 are each refused and keep the row; add on a dead entity is false; the
  tone names read "Clip" and "Soft".
- `scene/scene.md`: entries for the two headers; `scene/src/src.md`, `scene/tests/tests.md`: one each
  for the new files. Each at most 300 characters.

## Done when
The test `scene/sky` passes, and `scene/light_blocker` still passes, after the folder's build.
