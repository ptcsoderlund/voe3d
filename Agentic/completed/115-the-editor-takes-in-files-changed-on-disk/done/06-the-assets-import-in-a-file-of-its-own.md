# 06 — Move the Assets panel's import into a file of its own
folder: editor
after: 05
decisions: 0168

## Change
`editor/src/assets_panel.c` is 805 lines, and card 08 changes its import.

- Move `voe_editor_assets_import`, and the static helpers only it uses (`assets_made` among them),
  into a new file, `editor/src/assets_import.c`.
- Give the new file its own header comment, drawn from the moved code's comments: what importing a
  file into the shown folder does.
- Change no behaviour. `assets_panel.h` keeps the declaration.
- `editor/src/src.md`: add the new file's entry; the panel's entry no longer names import.

## Done when
- `test $(wc -l < editor/src/assets_panel.c) -lt 800` and
  `grep -q voe_editor_assets_import editor/src/assets_import.c` both exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
