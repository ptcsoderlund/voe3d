# 23 — The editor source index entries fit their cap
folder: editor/src
after: 22
decisions: 0168

## Change
`editor/src/src.md`: six entries are over the 300-character cap: `assets_panel.h`
(321), `interface.c` (348), `view_passes.h` (308), `view_passes.c` (379),
`models.h` (329), `undo.c` (328). Cut each to one sentence under 300 characters.
For each, if the cut drops a point not already in that file's header comment
(`editor/src/<name>`), add the point to the header comment. Read only those six
headers; no code change.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints `FINDINGS: 0`.
