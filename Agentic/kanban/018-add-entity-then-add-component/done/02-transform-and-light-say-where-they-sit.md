# 02 — Transform and light say where they sit in Add component
folder: scene
decisions: 0168, 0217, 0218, 0221

## Change
Each registration sets its type's menu path with `voe_ecs_component_menu_set` (`ecs/include/ecs/component.h`),
right after its `voe_ecs_component_default_set`:

- `scene/src/transform_system.c` — `"Transform"`.
- `scene/src/light_system.c` — `"Rendering / Light"`.
- Identity (`identity_system.c`) and camera (`camera_system.c`) set none: identity is on every authored
  entity and the camera is never offered (0218). Do not open those two `.c` files.

Headers: `scene/include/scene/transform_system.h` and `scene/include/scene/light_system.h` each gain one point
saying where the type sits in Add component and that the path is registered with the type (0221).
`scene/include/scene/camera_system.h` gains one point: it sets no path, so Add component never offers a camera.
`scene/scene.md` — the entries for those three headers each gain the phrase.

Tests, one check each, in the existing registration test of each file:
- `scene/tests/transform.c` — `voe_ecs_component_menu` of the transform type equals `"Transform"` (strcmp).
- `scene/tests/light.c` — equals `"Rendering / Light"`.
- `scene/tests/camera.c` and `scene/tests/identity.c` — reads NULL.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `scene` exits 0, with the four checks above in
`scene/transform`, `scene/light`, `scene/camera` and `scene/identity`.
