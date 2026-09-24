# 03 — A `game` folder builds the world and runs the scene through its camera
folder: game
decisions: 0168, 0175, 0234, 0237

## Change
New library folder `game` (0237 point 2); create it with its four-line `CMakeLists.txt` (shape of
`app/CMakeLists.txt`, `voe_module(game DEPENDS app 3d render scene ecs platform math base)`),
`game/game.md`, `include/game/game.md`, `src/src.md`, `tests/tests.md`. As 0175 says, add its row to
`voe_allowed_deps` in `cmake/voe.cmake` after `app`'s (`base math ecs scene platform render 3d app`,
with a comment: the loop a shipped game runs, no descriptions needed, never `authoring`) and
`add_subdirectory(game)` after `app` in the root `CMakeLists.txt`.

- `include/game/world.h` — `VOE_GAME_WORLD_MAX_DRAWN` 64, `VOE_GAME_WORLD_AUTHORED` 32 and
  `voe_ecs_world *voe_game_world_new(voe_base_arena *arena);`: the eight types a project's world
  holds, with the limits and capacities `world_new` and the defines above it in
  `editor/src/project.c` give today (read those lines only; the editor's 32 scene rows become
  `VOE_GAME_WORLD_AUTHORED`). Header: the one list, used by the editor and the game, so the cook can
  never name a type the game did not register.
- `include/game/scene.h` — includes the eight types' component headers and `ecs/component.h`, and
  declares `[[nodiscard]] bool voe_game_scene_build(voe_ecs_world *world);`. Header: defined by the
  cooked `scene.c` in a project's game tree, not by this folder (0237); the include the cook is
  handed.
- `include/game/frame.h` + `src/frame.c` — `[[nodiscard]] bool voe_game_frame(voe_app *app,
  voe_ecs_world *world, const voe_3d_shapes *shapes, voe_base_arena *scratch, voe_platform_size
  size);`: the structural queue applied, transform, identity and light systems run, the shape system
  run, a draw opened, one window pass through the scene camera (`voe_3d_draw_system_frame` with
  `size`), the draw system run, pass and draw closed. `VOE_GAME_CAPACITIES`: the device capacities
  that pass needs, from `VOE_GAME_WORLD_MAX_DRAWN` and `VOE_3D_SHAPES_*` (read
  `editor/src/view_passes.h`'s header for how the editor sizes them).
- `include/game/run.h` + `src/run.c` — `VOE_GAME_WIDTH` 1280, `VOE_GAME_HEIGHT` 720 (0234) and
  `int voe_game_run(const char *title);`: arenas, `voe_app_new` with the title, the world, 
  `voe_game_scene_build`, `voe_3d_shapes_upload`, then frames until the window is closing, skipping a
  minimised one; 0 on a close, 1 with a stderr line on any refusal. Header: no quit key, Escape is
  the game's (0234). `run.c` is the only file naming `voe_game_scene_build`.
- `tests/world.c` — each of the eight keys resolves through `voe_ecs_component_type` on a fresh world.
- `tests/frame.c` — a headless app (`voe_app_new_headless`, 128×72, `VOE_GAME_CAPACITIES`), a world
  with a camera, a light and one shape made through their typed creation calls, shapes uploaded;
  two `voe_game_frame` calls return true and `voe_app_capture_png` of the window target writes a
  file in the working directory. Neither test names `voe_game_scene_build`.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `game` exits 0 (both tests listed and passing),
and `cmake -P check.cmake` exits 0.
