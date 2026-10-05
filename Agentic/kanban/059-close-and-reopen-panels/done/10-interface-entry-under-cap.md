# 10 — The `interface.c` index entry fits the cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the entry for `interface.c` (four lines, 363 characters)
is over the 300-character entry cap. Rewrite it as one sentence under 300
characters naming what the file is for: one `ui` frame per root, and the one
read of the frame's clicks where commands are carried out. Drop the list of
whose clicks they are (bar, browser, Preferences, Project, picker, prefab, ×,
Panels rows, Panels list drawn last).

`editor/src/interface.c`: read its header comment only. Every point dropped
from the entry must already be in that header; one that is not (the × and
the Panels row toggling a root, the Panels list drawn last, the drag ghost,
Play and Ship polled once a frame) gets a phrase added there. No code change.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints no
FINDING for `editor/src/src.md`.
