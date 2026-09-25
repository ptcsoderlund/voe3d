# 16 — scene declares its component keys imported
folder: scene
decisions: 0168, 0245

## Change
Point 2 of 0245. A project reads `voe_scene_transform_key` (game/example/Code); on Windows that
read needs dllimport.

- `scene/include/scene/transform_component.h`, `camera_component.h`, `light_component.h`,
  `identity_component.h` — include `base/imported.h`; each `extern const struct voe_ecs_key
  voe_scene_<name>_key` gains `VOE_BASE_IMPORTED` after `extern`. No prose change unless a header
  says how its key is declared.

## Done when
1. `checks.sh --folder scene` prints `FINDINGS: 0`.
