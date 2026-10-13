# 15 — The sky row holds only the sky's look
folder: scene
after: none
decisions: 0168, 0406

## Change
0406 point 1: `voe_scene_sky`'s fields are replaced; `light`, `exposure` and `tone` leave it (card 16
gives exposure and tone their own row; 3d and game follow in cards 17 and 18). Read
`scene/include/scene/sky_component.h`, `scene/include/scene/sky_system.h`,
`scene/src/sky_component.c`, `scene/src/sky_system.c`, `scene/tests/sky.c`, and the `fill_colour`
field of `scene/include/scene/light_component.h` for a COLOUR field.

- `scene/include/scene/sky_component.h`: fields `zenith`, `horizon`, `ground` (`voe_math_float3`,
  COLOUR), `horizon_height`, `sun_size`, `sun_glow`, `sunset`, `haze` (FLOAT32), in that order; the
  tone names and constants and `VOE_SCENE_SKY_EXPOSURE_MAX` go, `VOE_SCENE_SKY_SUN_SIZE_MAX` 10 comes.
  Header points: the row holds how the sky looks and no light (its light on surfaces is the first
  directional light's fill, its sun that light's direction and colour); each field's meaning, unit
  and bounds as 0406 point 1; the sky is as bright as authored, only the sun's disc and glow
  brighter; the row and the first-row rules stay.
- `scene/src/sky_component.c`: the tone names go.
- `scene/include/scene/sky_system.h`, `scene/src/sky_system.c`: the default row of 0406 point 1; the
  drain refuses a number not finite, a colour channel below nought, a horizon height, sun glow or
  sunset outside 0 to 1, a sun size outside 0 to 10, a haze below nought; `add` asserts on the same.
  Headers and the usage example follow.
- `scene/tests/sky.c`: the default row is 0406's; a replace changes the horizon after a run; each
  refusal above is refused and keeps the row (a NaN zenith, a negative ground channel, horizon
  height 1.5, sun size 11, sunset −0.1, infinite haze); the tone-name case goes.
- `scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`: the sky entries say so. Each at most
  300 characters.

## Done when
The test `scene/sky` passes after the folder's build.
