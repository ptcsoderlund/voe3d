# 01 — Clouds are a row that drifts
folder: scene
after: none
decisions: 0168, 0480, 0492

## Change
0492 points 1 and 2. Built as the Haze row is, plus a runtime row and a run that takes seconds.
Read as the pattern `scene/include/scene/haze_component.h`, `scene/include/scene/haze_system.h`,
`scene/src/haze_component.c`, `scene/src/haze_system.c`, `scene/tests/haze.c`; for the named
UINT32 `scene/include/scene/picture_component.h` and `scene/src/picture_component.c` (`tone`);
for a runtime-only table the header of `scene/include/scene/prefab_system.h`; for a run that
steps a clock the headers of `3d/include/3d/water_system.h` and `3d/include/3d/water_component.h`.

- `scene/include/scene/clouds_component.h` (new): `VOE_SCENE_CLOUDS_FLAT` 0, `_VOLUMETRIC` 1 and
  `voe_scene_clouds_kind_names`; `voe_scene_clouds`, a described struct of `kind` UINT32,
  `coverage`, `height`, `thickness`, `wind_direction`, `wind_speed`, `shadow_strength`, all
  FLOAT32 after `kind`; its key, `_get`, `_count`, `_rows`, `_entities`; `voe_scene_clouds_drift`
  (`double x`, `double z`), its key and `_get`. Header points: having the row is clouds on; the
  first row on is used (0440); each field's meaning and bounds as 0492 point 1, thickness read by
  Volumetric only, wind degrees towards which it blows; the drift is how far the clouds have
  moved, runtime-only, never saved; shadow strength is the clouds' own, not the light's fill;
  written only through `scene/clouds_system.h`.
- `scene/include/scene/clouds_system.h` (new): `voe_scene_clouds_register(world, capacity)`
  (default row as 0492 point 1, also the unsaid row; no needed type; menu "Rendering / Clouds";
  the intent as its replace; the drift table runtime-only, same capacity),
  `[[nodiscard]] bool voe_scene_clouds_add(...)` asserting on a row the run would refuse,
  `voe_scene_clouds_intent { entity; clouds; }`, `[[nodiscard]] bool voe_scene_clouds_submit(...)`,
  `void voe_scene_clouds_system_run(voe_ecs_world *world, float seconds)`: drains replaces,
  refusing a number not finite, a kind past Volumetric, coverage or shadow strength outside 0 to
  1, thickness or wind speed below 0, keeping the row, one stderr line each; adds a drift at 0 to
  each row lacking one and drops drifts whose row is gone; moves each drift by wind × seconds in
  double. Asserts seconds ≥ 0. Header: usage, the run in order, the refusals, called where water's
  run is so edit and play drift alike.
- `scene/src/clouds_component.c`, `scene/src/clouds_system.c` (new), each with its header.
- `scene/tests/clouds.c` (new): registration (menu path, default row, fields in order and kinds,
  the drift runtime-only); add and read back; a replace changes `coverage` after a run; each
  refusal above keeps the row; a run adds a drift at 0; wind 90 at 10 m/s for two runs of 0.5 s
  moves the drift x by 10 and z by 0 (within 1e-6); wind 0 moves z by −speed × seconds; removing
  the row drops its drift after a run; add on a dead entity is false.
- `scene/scene.md`, `scene/include/scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`: an
  entry for each new file beside the Haze's, each at most 300 characters.

## Done when
The test `scene/clouds` passes after the folder's build.
