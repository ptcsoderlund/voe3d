# 30 — File rows in the Assets panel are drawn as folder rows are
folder: editor
after: none
decisions: 0168, 0194

## Change
Bug 04: model, prefab and picture labels in the Assets panel are brighter than all other text,
because `voe_editor_assets_draw` opens each such row as a selected choice (drawn inverted, 0194),
the way the Scene list marks its selected row. Every row is to look like a folder row: an
unselected choice, so its text takes the theme's colour and follows the contrast slider. Read the
header of `editor/src/assets_panel.h`; change these files and no others:

- `editor/src/assets_panel.c`, `voe_editor_assets_draw`: the row's `voe_ui_choice_begin` is never
  selected, for any kind of row; drop the comment above it that says the row is marked. The
  `model`, `prefab` and `picture` flags stay: assets_drag.h and the clicks read still use them to
  hold and fire a row.
- `editor/src/assets_panel.h`: the MODEL ROW, PREFAB ROW and PICTURE ROW paragraphs no longer say
  drawn marked; each says what makes the row (its ending) and what it is held or fired for, and
  one point says every row is drawn alike, the kind shown by the name's ending alone (0194: no
  colour meaning, the bug's ask).
- `editor/src/src.md`: the `assets_panel.h` entry drops "model, prefab and picture rows marked".

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0,
`! grep -n -i "marked" editor/src/assets_panel.h` exits 0, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/c.png" && test -s "$d/c.png"`
exits 0.

Human's: bug 04's steps 2 and 3 in `examples/tank_game` — file labels match folder labels and the
Inspector's text, and all dim and brighten together with the contrast slider.
