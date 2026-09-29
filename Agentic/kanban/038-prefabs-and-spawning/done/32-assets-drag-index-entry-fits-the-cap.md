# 32 — The assets_drag.h index entry fits the cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the `assets_drag.h` entry is 380 characters, cap 300.
Cut it to one sentence under 300 characters naming what the file does: a held
model or prefab row from the Assets panel, and what its release does over a
view, over the Inspector, and elsewhere.

Any detail the cut drops that is not already in the header comment of
`editor/src/assets_drag.h` (a part is not swapped; no prefab placed while a
prefab is open; the refused ghost) moves into that header comment. Only
comments change; no code, no other entries.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints no
finding naming `src.md`.
