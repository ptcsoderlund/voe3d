# 01 — The light shines along its transform and has a fill
folder: scene
decisions: 0168, 0273

## Change
The public surface changes; `3d`, `authoring`, `game`, `dev` and `editor` are fixed by later
cards, not this one.

- `scene/include/scene/light_component.h`: fields become `colour` (COLOUR), `intensity`
  (FLOAT32), `fill_colour` (COLOUR), `fill_intensity` (FLOAT32); `direction` goes. Add
  `voe_math_float3 voe_scene_light_direction(voe_math_quat rotation)` — the unit direction the
  light travels for that rotation, its −Z — and
  `voe_math_quat voe_scene_light_facing(voe_math_float3 direction)` — the shortest-arc rotation
  whose −Z is that direction, of any length but nought (asserts on nought; along +Z picks a
  half turn about Y). Rewrite the header: the direction is the transform's (0222, 0271), −Z as
  the camera looks, a light with no transform shines along −Z, what the fill is (a flat
  unshadowed lift of shadowed sides, 0 is none), the colours linear.
- `scene/src/light_component.c`: the two conversions.
- `scene/include/scene/light_system.h`, `scene/src/light_system.c`: register asserts a transform
  table is registered and sets the light's need of a transform and its intent as its replace
  (as `camera_system.c` does); default row white, 1, white, 0. Nothing is normalized any more;
  the drain keeps the last valid row with one stderr line on a non-finite number, a colour
  channel outside [0, 1] or a negative intensity. `add` stores the row as given (same
  validation asserts). Rewrite the header's normalization paragraphs and the usage sketch.
- `scene/tests/light.c`: cover the default row, the need and the replace being set, an intent
  landing, each refused intent keeping the row, both conversions (straight down, the round
  trip of a few directions within 1e-5, the +Z case).
- `scene/scene.md` light entries and `scene/src/src.md` if it names direction: say what the
  headers now say. `scene/tests/tests.md` for `light.c`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_scene voe_test_scene_light &&
ctest --test-dir build/debug -R "^scene/"` exits 0.
