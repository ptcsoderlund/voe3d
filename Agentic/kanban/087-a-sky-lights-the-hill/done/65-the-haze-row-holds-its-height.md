# 65 — The Haze row holds its height
folder: scene
after: none
decisions: 0168, 0418

## Change
Bug 09: 0418 point 1, the Haze row gains `height`. Read the headers of
`scene/include/scene/haze_component.h`, `scene/include/scene/haze_system.h`,
`scene/src/haze_system.c` and `scene/tests/haze.c`.

- `scene/include/scene/haze_component.h`: `float height` after `start` in the fields list,
  FLOAT32. Header points: the fields' bounds sentence no longer covers all three (`haze` and
  `start` finite and at least 0, `height` any finite); `height` is the world height, metres, y
  up, that the haze layer reaches; it veils the sky (and the backdrop with no sky row) seen
  through the layer, nothing else; surfaces still fade by distance only (0418 points 1 to 4).
- `scene/include/scene/haze_system.h`, `scene/src/haze_system.c`: the default row's `height` is
  1000 (also the unsaid row); the drain refuses a `height` not finite and keeps the row, any
  finite number taken; `add` asserts the same. The default-row comment, usage example and refusal
  paragraph name it.
- `scene/tests/haze.c`: the description lists three fields in order, `height` last, FLOAT32; the
  default row has `height` 1000; a replace with `height` −50 is taken; a NaN and an infinite
  `height` are each refused and keep the row. The header's list follows.
- `scene/scene.md`, `scene/include/scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`: the
  haze entries name the height where they name the fields, each one sentence of at most 300
  characters.

## Done when
The test `scene/haze` passes after the folder's build.
