# 26 — The editor source index entries fit the cap
folder: editor/src
decisions: 0168

## Change
`editor/src/src.md`: eight entries are over the 300-character cap. Cut each to
one sentence under 300 characters naming what the file does:
`project.h` (389), `topbar.h` (335), `scene_list.h` (350), `assets_panel.h`
(359), `interface.c` (341), `inspector.c` (348), `view_passes.c` (307),
`entities.h` (305).

Any detail a cut drops that is not already in the header comment of that file
(`editor/src/<name>`) moves into that header comment. Only comments change; no
code, no other entries.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints no
finding naming `src.md`.
