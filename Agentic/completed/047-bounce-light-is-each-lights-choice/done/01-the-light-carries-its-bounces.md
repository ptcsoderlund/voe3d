# 01 — The light carries its bounces
folder: scene
after: none
decisions: 0168, 0316, 0317, 0319

## Change
0319 points 1 and 2. Read `scene/include/scene/light_component.h`,
`scene/include/scene/light_system.h`, `scene/src/light_component.c`,
`scene/src/light_system.c`, `scene/tests/light.c`, and, for the named-field
pattern only, `3d/include/3d/shape_component.h` (its kind names and
`VOE_3D_SHAPE_NAMES`).

- `light_component.h`: `VOE_SCENE_LIGHT_FIELDS` gains `bounces` (`uint32_t`,
  UINT32) after `fill_intensity`; `VOE_SCENE_LIGHT_BOUNCES_MAX` 1; an extern
  `VOE_BASE_IMPORTED` names array `voe_scene_light_bounces_names` of
  `VOE_SCENE_LIGHT_BOUNCES_MAX + 1` entries; `VOE_SCENE_LIGHT_NAMES(N)`
  naming `bounces`; the struct declared through
  `VOE_BASE_DESCRIBE_STRUCT_NAMED`. Header points: how many times the
  light's own light bounces back off what it lit, 0 none and free (0316),
  1 the bounce of 046; 051 raises the maximum (0317); per light, not per
  scene; named so a tool offers exactly the allowed counts.
- `light_component.c`: the names array, "0" and "1".
- `light_system.h`: the default row's bounces 0 in the register comment; a
  count past the maximum joins what `add` asserts on and the drain refuses.
- `light_system.c`: the validity check that `add` and the drain share
  refuses `bounces > VOE_SCENE_LIGHT_BOUNCES_MAX`; the default row stays
  zero there.
- `scene/tests/light.c`: the default row's bounces is 0; an intent with
  bounces 1 lands; one with bounces 2 is refused and the row keeps 1; with
  descriptions compiled in, the description names `bounces` with two
  values "0" and "1". Fix any size or field-count check the new field breaks.
- `scene/scene.md` (light_component.h's entry gains the bounces count),
  `scene/src/src.md` and `scene/tests/tests.md` (light.c's entry gains the
  bounces checks) where their entries no longer say what the file does.

## Done when
`ctest --test-dir build/debug -R '^scene/light$'` passes, including the
four bounces checks named above.
