# 09 — Refresh builds a project's code and swaps it in; Play refreshes first
folder: editor
decisions: 0168, 0240, 0241, 0242

## Change
Point 7 of 0242 and the failure half of point 8. Read the headers of `editor/src/play.h`,
`editor/src/session.h`, `editor/src/game_tree.h`, `editor/src/code.h`, `editor/src/project.h`,
`editor/src/startup.h`, `editor/src/scene.h` and `editor/src/inspector_edit.h`.

- `editor/src/refresh.h`, `editor/src/refresh.c` (new) — play.c's shape for the LIBRARY kind:
  stages IDLE, CONFIGURING, BUILDING; `_start(refresh, project, why)` refuses an untitled project,
  writes the tree, empties the log and starts a configure (none cached) or a build, output to the
  log; `_poll` returns RUNNING, BUILT or FAILED (a failed step: one stderr line naming the log);
  `_end`; `_label` ("Refresh" idle, "Refreshing" else).
- `editor/src/session.h`, `editor/src/session.c`:
  - `VOE_EDITOR_COMMAND_REFRESH`; fields `refresh`, `play_after`, `refresh_due`, `loads`.
  - `refresh_due` is set wherever a project with a folder lands (the browser's Open Confirm, a
    Save that gives an untitled project its folder, startup); REFRESH starts one now.
  - PLAY while idle: code in `Code/` → a refresh with `play_after`; none → 024's play. PLAY during
    that refresh ends it (Stop).
  - `bool voe_editor_session_step(voe_editor_session *, voe_editor_scene *)` — once a frame: a due
    refresh starts if the project has code; a running one is polled. BUILT: if
    `voe_editor_code_same`, nothing; else `voe_editor_code_open` with `++loads` and
    `voe_editor_project_code_set`, the selection re-found by its authored id (none if gone) and
    the picker and dropdown closed (scene.h); then Play if `play_after`. FAILED or a refused load/swap: the notice says so, no Play, the old world
    kept. True when the world was swapped.
  - `const char *voe_editor_session_play_label(const voe_editor_session *)` — "Building" while a
    `play_after` refresh runs, else `voe_editor_play_label`. Close ends a running refresh too.
  - The header: REFRESH and step paragraphs; PLAY's paragraph gains the refresh first.
- `editor/src/startup.c` — sets `refresh_due` when a project with a folder opens.
- `editor/src/main.c` — `voe_editor_session_step` at the top of the frame, before the undo take
  and the world step; on true, closes Add component's list (`voe_editor_inspector_add_close`). A capture does not count frames while a refresh runs, and gives up after 120 s of
  the frame clock with one stderr line and exit 1. Every place the Play label is read uses
  `voe_editor_session_play_label` (interface.c's top-bar call, if that is where it is read).
- `editor/src/src.md` — entries for refresh.h and refresh.c; session's if its sentence changes.

## Done when
1. `checks.sh --folder editor` prints `FINDINGS: 0`.
2. In `p=$(mktemp -d)` with `cp -r game/example/. $p`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is empty,
   and `$p/Build/editor/loaded/project-1.so` exists.
3. In `q=$(mktemp -d)` with `cp -r game/example/. $q` and `echo '#error broken' >>` the first `.c`
   in `$q/Code`: the same capture exits 0, `$q/err` holds one line naming `build.log`,
   `grep -q error $q/Build/build.log` exits 0 and `$q/Build/editor/loaded` holds no file.
4. In `r=$(mktemp -d)` with `cp -r game/example/. $r` and `rm -r $r/Code`: the capture exits 0 and
   `$r/Build/editor` does not exist.
