# 06 — The point light has Cast shadows, off by default
folder: scene
after: none
decisions: 0168, 0324, 0325

## Change
The field, as 0324 point 1 gave the sun its own.
- `scene/include/scene/point_light_component.h`: `F(bool, cast_shadows, BOOL)` last in
  `VOE_SCENE_POINT_LIGHT_FIELDS`. Header: drop "it casts no shadow and bounces nothing"; say it
  casts when `cast_shadows`, off by default (0316), 3d choosing which casting lamps cast this frame
  (0325 point 5), and still bounces nothing. A scene saved before reads false, the default.
- `scene/include/scene/point_light_system.h`: the register comment's default row names
  `cast_shadows` false; the example need not change.
- `scene/src/point_light_system.c`: the default row has `.cast_shadows = false`; no unsaid row.
- `scene/tests/point_light.c`: the description lists `cast_shadows` BOOL last; the default row has
  it false; a replace with it true lands and one back to false lands. Entry in
  `scene/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R "^scene/point_light$"` passes.
