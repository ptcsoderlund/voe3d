# 01 — The sun and the point light carry bounces up to 3 and a bounce strength
folder: scene
after: none
decisions: 0168, 0316, 0326

## Change
0326 point 1. The Inspector, saving and the cook carry the fields with no code of their own (0319).
- `scene/include/scene/light_component.h`: `F(float, bounce_strength, FLOAT32)` last in
  `VOE_SCENE_LIGHT_FIELDS`; `VOE_SCENE_LIGHT_BOUNCES_MAX` 3u. Header: bounces goes to 3, each
  extra one carrying light one surface further; bounce strength scales the bounce only, never
  the direct light, 1 honest light, not negative; the "051 raises" phrase goes.
- `scene/src/light_component.c`: the names "0" to "3".
- `scene/src/light_system.c`: default row `bounce_strength` 1 (so the unsaid row too); a strength
  not finite or negative is a bad light, refused as a negative intensity is (`add` asserts, the
  drain keeps the last row and names the field).
- `scene/include/scene/light_system.h`: the default row and the refusals say strength.
- `scene/include/scene/point_light_component.h`: `F(uint32_t, bounces, UINT32)` and
  `F(float, bounce_strength, FLOAT32)` last; the struct described with
  `VOE_BASE_DESCRIBE_STRUCT_NAMED` naming `bounces` through the sun's
  `voe_scene_light_bounces_names` (include `light_component.h`), so it is the same dropdown.
  Header: drop "still bounces nothing"; a lamp bounces like the sun, 0 by default (0316).
- `scene/src/point_light_system.c`, `scene/include/scene/point_light_system.h`: default row
  bounces 0, strength 1; bounces past `VOE_SCENE_LIGHT_BOUNCES_MAX` or a strength not finite or
  negative refused as the sun's are.
- `scene/tests/light.c`: the description lists `bounce_strength` last; names "0".."3"; bounces 3
  lands and 4 is refused; strength −1 refused; default and unsaid rows have strength 1.
- `scene/tests/point_light.c`: the same for the lamp: fields, names, default 0 and 1, refusals.
- `scene/tests/tests.md`, `scene/src/src.md`: entries if they name the fields.

## Done when
`ctest --test-dir build/debug -R "^scene/(light|point_light)$"` passes.
