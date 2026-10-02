# 01 — A point light is a scene row that can flash
folder: scene
after: none
decisions: 0168, 0320

## Change
The folder's first point light, modelled on the sun's two headers and the emitter's runtime row.
Read `scene/include/scene/light_component.h`, `scene/include/scene/light_system.h`,
`scene/src/light_system.c` and `scene/tests/light.c` as the pattern, and
`3d/include/3d/emitter_component.h` for a runtime-only row and a control queue. Nothing else changes.

- `scene/include/scene/point_light_component.h` (new): `voe_scene_point_light` as a described field
  list (0320 point 1): `colour` COLOUR, `intensity` FLOAT32, `range` FLOAT32, `flash` FLOAT32,
  `flash_when_made` BOOL; its key; `voe_scene_point_light_get`, `_count`, `_rows`, `_entities` as the
  sun's. The runtime-only glow row `voe_scene_point_light_glow { float left; }` and its key.
  `float voe_scene_point_light_strength(const voe_ecs_world *world, voe_ecs_entity entity)`: intensity
  when steady; intensity × left / flash when flashing; 0 with no light, no glow row yet or none left.
  Header points: a lamp beside the sun, not a field on it; at the transform's world position, rotation
  and scale ignored, so a parent carries it; range is where it ends; intensity means what the sun's
  does; linear colours; the flash and what starts one (0320 point 2); no shadow (0301).
- `scene/include/scene/point_light_system.h` (new):
  - `void voe_scene_point_light_register(voe_ecs_world *world, uint32_t capacity)`: the table (default
    row 0320 point 1, needs a transform, menu "Rendering / Point light", whole-row intent as its
    replace), the flash queue, and the glow table runtime-only, each with room for `capacity`.
  - `bool voe_scene_point_light_add(world, entity, voe_scene_point_light light)`: direct, asserts on a
    light the drain would refuse; false when full or the entity is dead.
  - `voe_scene_point_light_intent { entity; light; }` and
    `bool voe_scene_point_light_submit(world, voe_scene_point_light_intent)`.
  - `bool voe_scene_point_light_flash_submit(voe_ecs_world *world, voe_ecs_entity entity)`.
  - `void voe_scene_point_light_system_run(voe_ecs_world *world, float seconds)`: applies the replaces
    in order (refusing per 0320 point 1, a flashing one's flash restarted), gives each light with no
    glow row one (full when `flash_when_made` and flashing, else dark), applies the flashes (one for a
    light it lacks or a steady one is ignored), then counts every glow down by `seconds`, never below 0.
    A flash given this run is not counted down this run.
  - Header points: why replace and flash are queued; what is refused; that the glow is the system's.
- `scene/src/point_light_component.c`, `scene/src/point_light_system.c` (new): the above; one file
  each, each with its header comment.
- `scene/tests/point_light.c` (new): registration (menu path, transform need, glow runtime-only); a
  refused replace keeps the row, for each refusal; a steady light's strength is its intensity; a
  flashing one is 0 until flashed, full after the run that flashes it, half after half its flash, 0
  after; `flash_when_made` is full on its first run; a replace re-flashes; a flash for a steady light
  or one with no row changes nothing.
- `scene/scene.md`: entries for the two headers; `scene/src/src.md` and `scene/tests/tests.md`: one
  each for the new files. Each entry at most 300 characters.

## Done when
The test `scene/point_light` passes, and `scene/light` still passes, after the folder's build.
