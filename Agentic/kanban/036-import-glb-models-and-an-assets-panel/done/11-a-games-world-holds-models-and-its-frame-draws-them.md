# 11 — A game's world holds models and its frame draws them
folder: game
decisions: 0168, 0277, 0237

## Change
Needs cards 06, 07 and 09. `voe_game_frame`'s signature changes; its callers are in this
folder. Nothing reads files yet: that is card 12.

- `game/include/game/world.h`, `game/src/world.c`: register `voe_3d_model` after the shape,
  room `VOE_GAME_WORLD_MAX_DRAWN`; `VOE_GAME_WORLD_TYPES` 12; "eleven" becomes "twelve" in
  both headers and in `game/include/game/game.md` and `game/src/src.md`.
- `game/include/game/frame.h`, `game/src/frame.c`:
  - The world step runs `voe_3d_model_system_run` after the shape system; the ORDER paragraph
    says so.
  - `voe_game_frame(voe_app *, voe_ecs_world *, const voe_3d_shapes *, const voe_3d_models
    *models, voe_base_arena *, voe_platform_size, float, const voe_ui_context *)` sets
    `frame.models` before the shadows and the draw; NULL draws no model.
  - `VOE_GAME_CAPACITIES` adds `VOE_3D_MODELS_VERTICES`, `_INDICES`, `_GEOMETRIES` and
    `_SHADINGS` to the shapes' numbers, and its objects count `2 * VOE_GAME_WORLD_MAX_DRAWN`
    where it counted `VOE_GAME_WORLD_MAX_DRAWN` (a model part is an object). Its comment says so.
- `game/src/run.c`: passes NULL for now.
- `game/tests/world.c`: expects twelve types and a model table. `game/tests/frame.c`: calls
  pass NULL; a new case builds a store with `voe_3d_models_new`, loads nothing, and a frame
  with it still draws. Update the lines in `game/tests/tests.md`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_game $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_game_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^game/"` exits 0.
