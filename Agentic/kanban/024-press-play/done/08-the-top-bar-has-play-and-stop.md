# 08 — The top bar has Play, which reads Building and then Stop
folder: editor
decisions: 0168, 0187, 0188, 0234, 0237
read: feature.md

## Change
The button, and the once-a-frame poll that moves Play on. `main.c` is not touched.

- `editor/src/topbar.h`, `editor/src/topbar.c` — `voe_editor_topbar` gains `play_button`.
  `voe_editor_topbar_draw` gains `const char *play`, the label, drawn as a button after Save and
  before Preferences; `voe_editor_topbar_clicks_read` answers `VOE_EDITOR_COMMAND_PLAY` when it
  fired. Header: the row's order names Play; the label is the caller's (the session's play state).
- `editor/src/interface.c` — `voe_editor_interface_draw` calls `voe_editor_play_poll` on
  `session->play` once, before any root is built, and hands `voe_editor_play_label` to the bar. The
  fired PLAY goes through `voe_editor_session_do` like the bar's other commands.
- `editor/src/interface.h` — a paragraph for the Play button: a button and its label, two nodes;
  its border and fill and the eight letters of "Building", its longest label, ten elements.
  `VOE_EDITOR_INTERFACE_NODES` 589 → 591, `VOE_EDITOR_INTERFACE_ELEMENTS` 5673 → 5683. The
  `session` paragraph of `voe_editor_interface_draw` says the play state is polled here, once a
  frame, because this is the one call every frame makes with the session.
- `editor/src/src.md` — the `topbar.h`, `topbar.c` and `interface.c` entries name Play.

Read the headers of `editor/src/play.h` and `editor/src/session.h`; in `interface.c`, only
`voe_editor_interface_draw` and what it calls on the bar.

## Done when
1. The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and with `XDG_CONFIG_HOME` at
   an empty `mktemp -d` folder `build/debug/editor/voe_editor --capture <that folder>/a.png --size
   1280x720` exits 0 and the picture's top bar reads New, Open, Save, Play, Preferences.
2. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.

For the human, on a desktop session, with a saved project holding a few shapes in different
colours and its camera: steps 1–9 of `## How to test` in `feature.md`. The first Play in a project
compiles the engine and reads Building for longer than five seconds; the five seconds hold from the
second Play on. After it, `git -C <project> status` (if the project is a repository) shows no
`Build/` file, and `<project>/.gitignore` lists `/Build/` and `/Cache/`.
