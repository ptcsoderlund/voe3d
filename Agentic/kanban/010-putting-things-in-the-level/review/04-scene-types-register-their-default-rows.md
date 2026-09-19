# 04 — Scene's types register their default rows
folder: scene
decisions: 0168, 0190

## Change
Each described type's register function also calls `voe_ecs_component_default_set` (card 03) with its default:
- transform (`src/transform_system.c`): position (0, 0, 0), rotation the identity quaternion, scale (1, 1, 1).
- identity: id 0, empty name.
- light: direction (0, -1, 0), colour (1, 1, 1), intensity 1.
- camera: eye (0, 0, 0), yaw 0, pitch 0, `fov_y` 1.0471976 (60°), near 0.1, far 1000.

Each register function's comment says it registers the default. Each `_system.h` header gets one sentence: the
default row is what "add at default" gives (0190).

## Done when
The folder's check passes (`checks.sh` for `scene`). `tests/transform.c`, `tests/identity.c`, `tests/light.c` and
`tests/camera.c` each check `voe_ecs_component_default` against the values above, field by field.
