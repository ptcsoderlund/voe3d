# 07 — The session plays the game, builds it first, and stops it
folder: editor
decisions: 0168, 0187, 0188, 0234, 0235, 0237

## Change
Play as a session command with its own state; the button that sends it is card 08.

- `editor/src/play.h`, `editor/src/play.c` — new.
  - `voe_editor_play_stage`: `IDLE` (zero), `CONFIGURING`, `BUILDING`, `RUNNING`.
    `voe_editor_play`: the stage, the one `voe_platform_process`, an arena of its own (NULL while
    idle) and the project's folder copied into it. Zeroed is idle.
  - `void voe_editor_play_start(voe_editor_play *play, const voe_editor_project *project,
    voe_editor_notice *why);` asserts idle. An untitled project: a notice asking to save it once
    first, nothing else (0237). Otherwise the arena is made, `voe_editor_game_tree_write` runs, and
    `voe_platform_process_start` gets the configure argv when `voe_editor_game_tree_configured` is
    false (CONFIGURING), the build argv when true (BUILDING). A refused write or start: the notice
    says why and play is idle again with its arena destroyed.
  - `void voe_editor_play_poll(voe_editor_play *play);` never blocks. CONFIGURING or BUILDING that
    ended 0 moves on (to BUILDING, then to RUNNING with the program from
    `voe_editor_game_tree_program` started); one that ended non-zero, or a program that will not
    start, prints one `voe_editor: …` line on stderr naming the step and goes idle — no notice
    (0234). RUNNING that ended goes idle. Idle is a no-op.
  - `void voe_editor_play_end(voe_editor_play *play);` ends whatever runs (`voe_platform_process_end`),
    destroys the arena, idle. Idle is a no-op.
  - `const char *voe_editor_play_label(const voe_editor_play *play);` "Play" idle, "Building" while
    configuring or building, "Stop" while running.
  - Header points: a program of its own so the editor keeps working (0187); the stages and why the
    first Play configures and compiles the engine (0235); a failed build is only a stderr line
    until errors are shown (0234); nothing here saves or marks the project (0188); the game sees the
    world as it was at the press, later edits never reach it.
- `editor/src/session.h`, `editor/src/session.c`
  - `VOE_EDITOR_COMMAND_PLAY` after `CLOSE`; `voe_editor_session` gains `voe_editor_play play`
    (zeroed session still valid).
  - `voe_editor_session_do` with PLAY: idle → `voe_editor_play_start(&session->play,
    session->project, &session->notice)`; otherwise → `voe_editor_play_end`. It never arms and
    answers false. A CLOSE that goes ahead calls `voe_editor_play_end` before answering true, so the
    editor never leaves a build or a game behind.
  - Header: the command list names Play; a paragraph on PLAY (never arms, never refused for unsaved
    work, a second press is Stop) and on CLOSE ending it.
- `editor/src/src.md` — `play.h` and `play.c` entries; the `session.h` and `session.c` entries
  name Play.

Read the headers of `editor/src/game_tree.h`, `platform/process.h`, `editor/src/notice.h` and
`editor/src/project.h`; in `session.c`, only `voe_editor_session_do`.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0.
