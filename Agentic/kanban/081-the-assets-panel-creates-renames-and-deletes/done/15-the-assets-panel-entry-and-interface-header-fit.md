# 15 — The Assets panel entry and interface.c's header fit their caps
folder: editor/src
after: none
decisions: 0168, 0378

## Change
Two findings in this folder; change no code.

- `editor/src/src.md`, entry `assets_panel.h` (line ~173): 392
  characters, cap 300. One sentence under 300 characters: the Assets panel
  as rows of `<project>/Assets/`, and the create, rename, delete and move
  it asks for. Any detail cut that the header comment of
  `editor/src/assets_panel.h` lacks goes there.
- `editor/src/interface.c`: header comment is 62 lines, cap 60. Bring it
  to 60 or fewer. The paragraphs that explain the order of reads inside
  `voe_editor_interface_draw` (the colour picker read before the
  Inspector's buttons; the Assets panel's prefab, naming and Delete
  requests; the right-button menu read after the panel) are the why of that
  one function: move the order-of-reads detail into the comment directly
  above `voe_editor_interface_draw`, leaving in the header a phrase per
  thing the file draws or reads. Nothing said is lost.

## Done when
`bash /home/ptcsoderlund/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints `FINDINGS: 0`.
