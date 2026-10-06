# 05 — The splash wait: a worker, its progress, and the shaders step
folder: game
after: 04
decisions: 0168, 0362, 0370

## Change
- `game/include/game/progress.h` and `game/src/progress.c` (new): the record a worker reports into
  and the main thread reads (0370 point 3): `voe_game_progress` with an atomic phase (a string the
  worker keeps alive), done, total and stop. Calls: `voe_game_progress_set(progress, phase, done,
  total)` and `voe_game_progress_stopped(progress)`, both doing nothing / answering false on NULL;
  `voe_game_progress_line(progress, char *buffer, size_t size)` writing "Preparing shaders 12/40",
  or the phase alone when total is 0. Header says who writes, who reads, and why atomics.
- `game/include/game/starting.h`, `game/src/starting.c`:
  - `typedef bool voe_game_starting_work(void *context, voe_game_progress *progress);`
  - `[[nodiscard]] bool voe_game_starting_wait(voe_app *app, voe_ui_context *ui,
    voe_base_arena *frame_arena, const voe_app_picture *splash, voe_game_starting_work *work,
    void *context)`: one `thrd_t` runs `work`; meanwhile starting frames with the progress line,
    the frame arena rewound after each; a window not visible draws none and waits on the window
    50 ms (`platform/window.h`). A false frame (closing): stop set, worker joined, false.
    Otherwise the work's answer once it has ended. Starting phase line: "Starting".
  - `[[nodiscard]] bool voe_game_starting_shaders(voe_render_device *device,
    const char *cache_path, voe_base_arena *scratch, voe_game_progress *progress)`, for a work:
    `voe_app_pipeline_cache_load` (none when the path is NULL), then `voe_render_device_prepare`
    a step at a time with "Preparing shaders" done/`voe_render_device_prepare_steps`, false before
    the next step once stopped or on FAILED, `voe_app_pipeline_cache_save` once PREPARED.
  - Header: the wait's two threads and what each may touch (the worker owns what the context
    names until the wait returns; the main thread touches only the app's window, ui and splash),
    the close within a step, the cache (0370). Keep `voe_game_starting_prepare` for now; card 08
    removes it.
- `game/tests/starting.c`: the wait with a work running `voe_game_starting_shaders` on a headless
  app leaves the device PREPARED and the cache file (a path under the working folder) written; a
  work answering false makes the wait false; the progress line for total 0 and for 3/7.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: entries.

## Done when
`ctest --test-dir build/debug -R '^game/starting$'` passes.
