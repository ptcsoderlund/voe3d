# 37 — A directional light sets its disc and glow
folder: scene
after: 36
decisions: 0168, 0411, 0412

## Change
0411 point 2 and 0412 point 6's light row: `disc_size` and `glow_size` join it. Read the headers of
`scene/include/scene/light_component.h`, `scene/include/scene/light_system.h`,
`scene/src/light_system.c` and `scene/tests/light.c`.

- `scene/include/scene/light_component.h`: `F(float, disc_size, FLOAT32)` and `F(float, glow_size,
  FLOAT32)` after `bounce_strength`; `VOE_SCENE_LIGHT_DISC_SIZE_MAX` 10. Header points: under a
  sky every directional light draws its own disc in its own colour at a fixed brightness, whatever
  its intensity, and a glow tinting the sky around it; `disc_size` is the disc's angular radius in
  degrees, 0 to the maximum, 0 no disc; `glow_size` 0 to 1, 0 no glow; two suns are two lights;
  without a sky neither does anything. The INTENSITY paragraph's "no exposure or tone mapping
  yet" is out of date: exposure and tone are the picture row's (scene/picture_component.h).
- `scene/include/scene/light_system.h`, `scene/src/light_system.c`: the default and unsaid rows
  both carry disc 1.5 and glow 0.3, so old scenes show a sun; the drain refuses a disc size
  outside 0 to 10 or a glow size outside 0 to 1, or either not finite; `add` asserts the same.
  The header's default-row and refusal lists follow.
- `scene/tests/light.c`: the default and the unsaid row have disc 1.5 and glow 0.3; disc 11,
  disc −1, glow 1.5 and a NaN glow are refused and keep the row; disc 0 and glow 0 are taken.
- `scene/scene.md`, `scene/include/scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`:
  the light entries say so, each one sentence of at most 300 characters.

## Done when
The test `scene/light` passes after the folder's build.
