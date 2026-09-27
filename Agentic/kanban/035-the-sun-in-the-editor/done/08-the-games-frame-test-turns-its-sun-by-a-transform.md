# 08 — The game's frame test turns its sun by a transform
folder: game
decisions: 0168, 0273

## Change
Needs cards 01–03. `game/src/world.c` already registers the transform table before the
light's and `game/src/frame.c` takes the light from `3d`, so a played game lights by the
sun's transform and fill with no change in `src/`.

- `game/tests/frame.c`, `build()`: the light entity gets a transform whose rotation is
  `voe_scene_light_facing((voe_math_float3){ -0.4f, -1.0f, -0.6f })` and a light of colour
  white, intensity 3, no fill, in place of `.direction`. Its header comment, if it speaks of a
  direction, says the sun is turned by its transform.
- Any other file under `game/tests/` or `game/src/` that builds a `voe_scene_light` with
  `.direction` (grep): the same change.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_game $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_game_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^game/"` exits 0.
