# 09 — The editor's start and scene loads run behind a live splash
folder: editor
after: 08
decisions: 0168, 0362, 0370
read: feature.md

## Change
- `editor/src/loading.h`, `editor/src/loading.c` (new): the editor's two works for
  `voe_game_starting_wait`, each a context struct of pointers and a work function (0370 point 4):
  - the start: `voe_game_starting_shaders` with
    `voe_app_pipeline_cache_path(scratch, "pipelines_editor.cache")`, then `voe_3d_shapes_upload`
    ("Uploading shapes"), then `voe_editor_models_update` with the progress;
  - a load: `voe_editor_session_load` ("Loading scene"), then `voe_editor_models_update` with the
    progress.
  Each worker uses a scratch arena of its own. The header says what the worker owns during the
  wait (session, scene, models, shapes) and that the main thread touches none of them until it
  returns.
- `editor/src/main.c`: the start's `voe_game_starting_prepare` and the shapes' upload become one wait
  on the start work, the start log's steps kept; the `load_due` path's splash frame and load become
  one wait on the load work. A false wait ends the program as a closed window does now; a failed
  shapes upload keeps today's message. The header's paragraph on the start and loads says the work
  is on a worker behind a live splash, and the cache.
- `editor/src/splash.h`: its example names the wait, not the prepare loop.
- `editor/src/src.md`: entries for `loading.h`, `loading.c`, and `main.c`'s.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`./build/debug/editor/voe_editor --capture shot.png examples/<any example folder>` writes
`shot.png` (find the binary and an example with `ls build/debug/editor examples`). Human: How
to test steps 1, 2, 3, 4 and 6 in `feature.md`, then step 5.
