# 12 — A game reads the models its world names
folder: game
decisions: 0168, 0277, 0266

## Change
Needs cards 04, 08 and 11. The loader of 0277 points 4 and 5, shared by the game and the
editor.

- New `game/include/game/models.h`, `game/src/models.c`:
  - `uint32_t voe_game_models_update(const voe_ecs_world *world, voe_3d_models *models,
    voe_render_device *device, const char *folder, voe_base_arena *scratch,
    const char **failed);` — for every model row with a non-empty path the store has no entry
    for: join `folder` and the path (`platform/path.h`), take its stamp, read it
    (`voe_platform_file_read` into `scratch`, rewound after each), load it; a file with no stamp
    or that will not read is `voe_3d_models_fail` with stamp 0. Returns how many failed this
    call; `*failed` (when not NULL) is the first one's path, the store's own copy. Each failure
    is one stderr line naming the path and the category.
  - `uint32_t voe_game_models_watch(voe_3d_models *, voe_render_device *, const char *folder,
    voe_base_arena *scratch, const char **failed);` — every entry's stamp asked again; a changed
    one (a failed entry whose file now has a stamp included) is re-read and loaded again; same
    return and lines.
  - Header points: called between frames only (a load waits for idle); the folder is the
    project's in the editor and the program's in the game; why a stamp and not a watcher API.
- `game/include/game/run.h`, `game/src/run.c`: the run makes a store, calls `_update` against
  the program's folder (the one `sound_folder` finds) after the scene is built and once a frame
  before `voe_game_frame`, passes the store to it, and clears and destroys it at the end. The
  ORDER paragraph says where.
- New `game/tests/models.c`, headless, skipping without a card: the test writes a minimal
  one-triangle `.glb` of its own (JSON and BIN chunks built in the test) into a scratch folder
  under the build tree; a world with a model row naming it updates with 0 failures and a
  loaded entry; a row naming a missing file gives 1 failure and names it; the file rewritten
  as text makes `_watch` report 1 and keeps the entry loaded; rewritten valid, `_watch`
  reports 0. List it in `game/tests/tests.md`; entries in `game.md` and `src/src.md`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_game $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_game_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^game/"` exits 0, `voe_test_game_models` among them.
