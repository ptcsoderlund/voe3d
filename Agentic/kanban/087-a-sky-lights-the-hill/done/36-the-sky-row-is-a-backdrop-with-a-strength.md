# 36 — The sky row is a backdrop with a strength
folder: scene
after: none
decisions: 0168, 0411, 0412

## Change
0411 point 3 and 0412 point 6's sky row: `sun_size`, `sun_glow` and `sunset` leave it, `strength`
comes. `3d` and `game` break until cards 38 and 39; do not touch them. Read the headers of
`scene/include/scene/sky_component.h`, `scene/include/scene/sky_system.h`,
`scene/src/sky_system.c` and `scene/tests/sky.c`.

- `scene/include/scene/sky_component.h`: fields `zenith`, `horizon`, `ground`, `horizon_height`,
  `strength`, `haze`, in that order; `VOE_SCENE_SKY_SUN_SIZE_MAX` goes (card 37 gives the light
  its own). Header points: the row is a backdrop and lights nothing; the lights draw their own
  discs and glows in it (scene/light_component.h); `strength` scales the backdrop and so the haze,
  never a disc, at least 0; a dark sky under a bright light is fine; game code animates the colours
  for a day; each other field as now.
- `scene/include/scene/sky_system.h`, `scene/src/sky_system.c`: default row with strength 1, the
  rest as now; the drain refuses a strength below 0 or not finite and no longer checks the three
  gone fields; `add` asserts the same. Headers and the usage example follow.
- `scene/tests/sky.c`: the default row has strength 1; a strength of −1 and of infinity are
  refused and keep the row; cases on the gone fields go.
- `scene/scene.md`, `scene/include/scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`:
  the sky entries say so, each one sentence of at most 300 characters.

## Done when
The test `scene/sky` passes after the folder's build.
