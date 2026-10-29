# 01 — A fog volume is a placed box that drifts
folder: scene
after: none
decisions: 0168, 0480, 0493

## Change
0493 points 1 and 2. The Clouds row is the pattern for the fields, the drift and the run; the
light blocker is the pattern for a box placed by its transform and the transform it needs. Read
`scene/include/scene/clouds_component.h`, `scene/include/scene/clouds_system.h`,
`scene/src/clouds_component.c`, `scene/src/clouds_system.c`, `scene/tests/clouds.c`, and the
headers of `scene/include/scene/light_blocker_component.h` and
`scene/include/scene/light_blocker_system.h`.

- `scene/include/scene/fog_volume_component.h` (new): `voe_scene_fog_volume`, a described struct
  of `size` FLOAT3, then `thickness`, `patchiness`, `wind_direction`, `wind_speed`,
  `godrays_strength`, all FLOAT32; its key, `_get`, `_count`, `_rows`, `_entities`;
  `voe_scene_fog_volume_drift` (`double x`, `double z`), its key and `_get`. Header points: a box
  placed, turned and scaled by its transform, size the whole edge before scale; fog only inside,
  fading softly to its faces, none outside (0480 point 4); each field's meaning and bounds as 0493
  point 1; godrays strength is how strongly the sun's beams show in it, the fog staying; an off
  entity is no fog; it draws, casts and collides with nothing; the drift is how far its patches
  have moved, runtime-only, never saved; written only through `scene/fog_volume_system.h`.
- `scene/include/scene/fog_volume_system.h` (new): `voe_scene_fog_volume_register(world,
  capacity)` (default row as 0493 point 1, also the unsaid row; the transform needed; menu
  "Rendering / Fog volume"; the intent as its replace; the drift table runtime-only, same
  capacity), `[[nodiscard]] bool voe_scene_fog_volume_add(...)` asserting on a row the run would
  refuse, `voe_scene_fog_volume_intent { entity; fog_volume; }`,
  `[[nodiscard]] bool voe_scene_fog_volume_submit(...)`,
  `void voe_scene_fog_volume_system_run(voe_ecs_world *world, float seconds)`: drains replaces,
  refusing a number not finite, a size component below 0, thickness, patchiness or godrays
  strength outside 0 to 1, wind speed below 0, keeping the row, one stderr line each; adds a drift
  at 0 to each row lacking one and drops drifts whose row is gone; moves each drift by wind ×
  seconds in double, as the clouds'. Asserts seconds ≥ 0. Header: usage, the run in order, the
  refusals, called where the clouds' run is.
- `scene/src/fog_volume_component.c`, `scene/src/fog_volume_system.c` (new), each with its header.
- `scene/tests/fog_volumes.c` (new): registration (menu path, the transform needed, default row,
  fields in order, the drift runtime-only); add and read back; a replace changes `thickness`
  after a run; each refusal above keeps the row; a run adds a drift at 0; wind 90 at 2 m/s for two
  runs of 0.5 s moves the drift x by 2 and z by 0 (within 1e-6); removing the row drops its drift
  after a run; add on a dead entity is false.
- `scene/scene.md`, `scene/include/scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`: an
  entry for each new file beside the Clouds', each at most 300 characters.

## Done when
The test `scene/fog_volumes` passes after the folder's build.
