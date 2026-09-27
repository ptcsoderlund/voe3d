# 25 — A cooked scene names the model with its declaration
folder: game
decisions: 0168, 0277

## Change
Bug 02: a scene with a placed model does not compile in the game tree. The cooked `scene.c`
includes only `<game/scene.h>` (authoring/scene_cook.h), and that header lacks the model
component's header, so `voe_3d_model_key` and `voe_3d_model` are undeclared. Fix it once, in the
header the cook is handed; change these files and no others:

- `game/include/game/scene.h`: include `<3d/model_component.h>` beside the other `3d/`
  component headers. Its comment gains one point: it includes the header of every type
  `game/world.h` registers, and `tests/world.c` is what keeps the two lists together.
- `game/tests/world.c`: include `<game/scene.h>` (with `<game/world.h>`) in place of every
  component header (`3d/...`, `physics/...`, `scene/...`), so each key the test names is
  declared only through what the cooked scene sees; a key missing from `scene.h` stops the test
  compiling. It never calls `voe_game_scene_build`, so it links without a `scene.c`. Its top
  comment says why it includes `scene.h`.
- `game/tests/tests.md`: the `world.c` entry adds that its keys come through `game/scene.h`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_game $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_game_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^game/"` exits 0, `voe_test_game_world` among them, and `grep -c '3d/model_component.h'
game/include/game/scene.h` prints 1 while `grep -cE '#include <(3d|physics|scene)/'
game/tests/world.c` prints 0.
