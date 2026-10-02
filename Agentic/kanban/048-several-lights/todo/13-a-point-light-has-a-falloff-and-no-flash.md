# 13 — A point light has a falloff and no flash
folder: scene
after: none
decisions: 0168, 0321, 0322

## Change
The point light loses its flash and gains `falloff` (0321 points 1–2, 0322 points 1, 2, 4 and 5).
This changes the folder's public surface: 3d, game, editor and the tank game are fixed by their own
cards (15–19), not here. Read and change only the files below.

- `scene/include/scene/point_light_component.h`:
  - The field list becomes `colour`, `intensity`, `range`, `falloff` (FLOAT32), in that order;
    `flash` and `flash_when_made` go.
  - `VOE_SCENE_POINT_LIGHT_FALLOFF_LEAST` 0.25f and `VOE_SCENE_POINT_LIGHT_FALLOFF_MOST` 4.0f.
  - `voe_scene_point_light_glow`, its key and `voe_scene_point_light_strength` go: a reader draws
    with `intensity`.
  - Header points: the flash paragraph goes; game code that wants a flash fades the intensity through
    the replace (0321 point 2); falloff is how fast the light fades on the way to its range, the
    curve of 0322 point 1, 1 the default and 0320's look, low an even pool with a soft rim, high a
    bright core; a scene saved before it reads 1 and its flash keys are ignored with a warning
    (0322 point 4); a directional light has neither range nor falloff. No runtime-only row any more.
- `scene/include/scene/point_light_system.h`:
  - `voe_scene_point_light_flash_submit` goes.
  - `void voe_scene_point_light_system_run(voe_ecs_world *world)`: no seconds; applies the replaces
    in submission order, nothing else.
  - Register: the default row gains falloff 1; no flash queue, no glow table.
  - Refused: a non-finite number, a channel outside 0–1, a negative intensity, a range of nought or
    less, a falloff outside LEAST..MOST. `add` asserts on the same.
  - Header points: the usage block shows a fade as a replace with a lower intensity, and the run with
    no seconds; the flash and glow paragraphs go; the replace no longer restarts anything.
- `scene/src/point_light_component.c`: the glow key and the strength go.
- `scene/src/point_light_system.c`: the flash queue, glow table, glow making and count-down go; the
  run's signature as above; the falloff refusal. Header comment to match.
- `scene/tests/point_light.c`: the flash, glow and strength cases go. Keep registration (menu path,
  transform need), refused replaces, each keeping the row (add a falloff below LEAST, one above MOST,
  one NaN); add: the default row's falloff is 1; an accepted replace changes falloff and intensity
  after one run; a falloff of exactly LEAST and of MOST is accepted.
- `scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`: the entries for the four files and
  the test say falloff and no flash, glow or strength. Each at most 300 characters.

## Done when
The test `scene/point_light` passes, and `scene/light` still passes, after the folder's build, and
`! grep -n "flash_when_made\|point_light_glow\|point_light_strength\|flash_submit" scene/include/scene/*.h scene/src/*.c scene/tests/*.c`
exits 0.
