# 08 — The game window keeps its LODs
folder: game
after: 04, 07
decisions: 0168, 0500, 0501

## Change
Play and a shipped build both draw through `voe_game_run` → `game/src/run.c` → `voe_game_frame`
(`game/src/frame.c`). The window gets one LOD memory (`3d/include/3d/lods_kept.h`; read its header),
held as its bounce casters are (0501 point 1).

- `game/include/game/frame.h`: `voe_game_frame` gains `voe_3d_lods_kept *lods` after `casters`: the
  window's LOD memory, the caller's, NULL for none (one LOD whole, no dissolve). Its comment and the
  example at the top (line ~11) follow; `game/include/game/steps.h`'s example (line ~10) too.
- `game/src/frame.c`: when `lods` is not NULL, begin it with `voe_platform_clock_now()` once per
  frame drawn, before the shadows and the draws, and set it as the 3d frame's `lods`. The clock is
  wall time: a paused Play still finishes a dissolve (0501 point 2).
- `game/src/run.c`: beside the window's `casters` (line ~396) make one memory with
  `voe_3d_lods_kept_create(arena)`, kept for the whole run across restarts, and pass it through
  `run_frames` to `voe_game_frame`. The file's top list (line ~14) names it beside the casters.
- `game/tests/frame.c`: every existing call passes NULL for `lods`. Add
  `a_window_memory_keeps_the_row_lod`: with models as the test at ~339 loads them and a memory from
  `voe_3d_lods_kept_create`, two frames drawn, then `voe_3d_lods_kept_find` with
  `voe_3d_lods_kept_row_key` of the model row's entity is true.
- `game/include/game/game.md` (`frame.h`) and `game/src/src.md` (`run.c`): entries mention the
  window's LOD memory.

## Meaning
- Holds: each model copy and foliage copy keeps, per view, its LOD and the moment it last swapped
  (0500 pt 1).
- Gap answered by 0501 point 2: the dissolve runs on the platform steady clock; pause does not stop it.

## Done when
The test `game/frame` passes, and `grep -q voe_3d_lods_kept_create game/src/run.c` exits 0.
