# 12 — Play and Ship cook stale copies first
folder: editor
after: 08, 10, 11
decisions: 0168, 0470
read: feature.md

## Change
0470 point 6. Play and Ship copy `Cache/` into their build (card 10), and the game trusts those copies,
so the editor cooks every stale one first. With none stale there is no splash and no delay.

- `editor/src/session.h` / `session.c`:
  - `session_play_start` and `session_ship_start` (near lines 90 and 114 of `session.c`) no longer start.
    For a titled project they set a new `cook_due` and remember which build follows (Play or Ship). An
    untitled project starts as today, which gives its notice.
  - A new `void voe_editor_session_cooked(voe_editor_session *session)` clears `cook_due` and starts the
    remembered build through the old start code.
  - The session header's Play and Ship paragraphs say that a build waits one frame for the cook, and why
    nothing can press in between: the waits run at the frame's start, before input.
- `editor/src/loading.h` / `loading.c`:
  - a fourth work context, `voe_editor_loading_cook`: the folder, the `Assets/...` paths of the `.glb`
    files, their count, the materials table (`voe_game_materials`) and the worker's scratch;
  - `voe_editor_loading_cook_work`, which calls `voe_game_models_cook`;
  - the header gains a COOK paragraph.
- `editor/src/waits.c` / `waits.h` (card 11): when `session.cook_due`, on the main thread:
  1. List every `.glb` under `<folder>/Assets/` with `voe_editor_game_tree_find`
     (`editor/src/game_tree_find.h`). Prefix each path to `Assets/...` as model rows name them.
  2. Take the table from `voe_editor_materials_table(voe_editor_models_materials(models))`.
  3. Ask `voe_game_models_stale`.
  4. If it is above 0, run the cook work under `voe_game_starting_wait`. A false wait ends the program as
     the load wait does.
  5. Call `voe_editor_session_cooked`.

  A find that fails puts its notice and starts the build anyway.
- `editor/src/src.md`: the session, loading and waits entries name the cook before Play and Ship.

## Done when
`grep -q voe_game_models_stale editor/src/waits.c && grep -q voe_editor_session_cooked editor/src/session.c`
exits 0.

The human then runs `## How to test` of `feature.md`, on the hill:
1. Open the hill and note the splash's "Loaded in" time. Press Play and note that time too.
2. Close the editor, delete the project's `Cache/` and open again. The splash says "Preparing models"
   and the time beats step 1.
3. Close and open again. It shows "Loading models" only and is much faster than step 1. Press Play: it is
   much faster than step 1 too.
4. Re-export one model with a visible change. It shows within a second or so. Close and open: only that
   model is prepared.
5. Import textures on a model at a smaller size. The new pictures show. The next open prepares only that
   model.
6. Ship and run the shipped game. Nothing is prepared, and it looks as in Play.
