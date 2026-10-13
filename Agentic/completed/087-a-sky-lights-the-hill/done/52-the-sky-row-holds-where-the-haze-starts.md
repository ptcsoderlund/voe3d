# 52 — The sky row holds where the haze starts
folder: scene
after: none
decisions: 0168, 0414

## Change
0414 points 1 and 2 in the sky row. Read the headers of `scene/include/scene/sky_component.h`,
`scene/include/scene/sky_system.h`, `scene/src/sky_system.c` and `scene/tests/sky.c`.

- `scene/include/scene/sky_component.h`: `float haze_start` after `haze` in
  `VOE_SCENE_SKY_FIELDS`, FLOAT32. Header points: `haze` is the half distance past the start;
  `haze_start` is metres within which no surface takes haze, at least 0, 0 the fade from the eye.
- `scene/include/scene/sky_system.h`, `scene/src/sky_system.c`: the default row's `haze_start` is
  100 (it is also the unsaid row, as for every field); the drain refuses one below 0 or not finite
  and keeps the row; `add` asserts the same. The refusal paragraph and the usage example name it.
- `scene/tests/sky.c`: the description lists seven fields in order, the last `haze_start`; the
  default row has `haze_start` 100; a `haze_start` of −1 and of infinity are each refused and keep
  the row. The header's list follows.
- `scene/include/scene/scene.md`, `scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`: the
  sky entries say so where what they say changed (the field count in `tests.md`), each one sentence
  of at most 300 characters.

## Done when
The test `scene/sky` passes after the folder's build.
