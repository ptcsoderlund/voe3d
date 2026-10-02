# 02 — The cook carries the bounces to the game
folder: authoring
after: 01
decisions: 0168, 0319

## Change
0319 point 2: the writer, reader and cook take `voe_scene_light.bounces`
through its description with no code of their own; only a test's expected
text changed. Read `authoring/tests/scene_cook.c` and
`scene/include/scene/light_component.h`. No file under `authoring/src/`
changes; if a test below fails for a reason other than expected text, stop
and block the card with what failed.

- `authoring/tests/scene_cook.c`: the sun the test adds (around line 87)
  gets `bounces = 1`, and the expected source for its light row (around
  line 162) carries the bounces field as the cook spells a UINT32, so the
  test proves a chosen bounce reaches the game's compiled scene.
- Run the whole authoring suite; any other exact-text expectation that now
  holds a light's bounces is updated the same way.
- `authoring/tests/tests.md`: `scene_cook.c`'s entry says the sun's bounces
  is cooked.

## Done when
`ctest --test-dir build/debug -R '^authoring/'` passes, with
`authoring/scene_cook` expecting the sun's bounces of 1 in the cooked source.
