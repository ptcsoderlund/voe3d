# 46 — The editor sources' entries fit their cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: seven entries are over the 300-character cap —
`frame_commands.c` (373), `game_tree.c` (303), `browser.h` (316),
`dock_walk.c` (311), `assets_drag.h` (394), `models.h` (343), `models.c`
(336). Cut each to one sentence of at most 300 characters saying what the
file is for.

What the cut drops goes into that file's own header comment, unless the
header already says it; check before adding. Header lengths now:
`frame_commands.c` 9, `game_tree.c` 16, `browser.h` 57, `dock_walk.c` 50,
`assets_drag.h` 60, `models.h` 52, `models.c` 8, against a cap of 60.
`assets_drag.h` is full and `browser.h` nearly so: their dropped detail is
either already in the header or is reworded into it with no net lines. No code
changes; comments and `src.md` only.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints no
FINDING naming `editor/src/src.md`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines editor/src/{frame_commands.c,game_tree.c,browser.h,dock_walk.c,assets_drag.h,models.h,models.c}`
prints no "over cap".
