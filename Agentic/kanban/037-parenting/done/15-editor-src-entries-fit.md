# 15 — The editor source index entries fit the cap
folder: editor/src
decisions: 0168

## Change
`editor/src/src.md`: the entries `interface.c` (322 characters) and `scene.h` (320) are over the
300-character entry cap. Shorten each to one sentence under 300 characters that says what the
file does or holds. Open the header comment of `editor/src/interface.c` and `editor/src/scene.h`;
any point the entry drops that the header does not already make (the Scene list's drop carried
out in the frame's one click read; the row being dragged and the structural changes held in the
scene state) goes into that header. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints `FINDINGS: 0`.
