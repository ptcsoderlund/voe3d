# 22 — The picker stands beside the Inspector with no Duplicate
folder: editor
decisions: 0168, 0273

## Change
Bug 01: the sun's swatches fire, but the picker opens off the right edge of the window. The
picker's `left` (`voe_editor_picking`, scene.h) is taken from the Duplicate button's left
edge, and since card 11 a light's entity has no Duplicate button, so `left` is 0 and
interface.c anchors the picker's right edge past the surface. The camera's entity has none
either. The owner is the swatch branch of `voe_editor_inspector_buttons_read`; interface.c and
scene.c are right as they are and are not touched.

- `editor/src/inspector_buttons.c`, `voe_editor_inspector_buttons_read`, the fired-swatch
  loop: `left` becomes the left edge of `inspector->content` (the column everything the panel
  draws sits in, inspector.h) via `voe_ui_node_rect`, 0 only when that node is
  `VOE_UI_NODE_NONE`. The comment above the loop says the content column's left edge, not
  Duplicate's, and why: Duplicate is not drawn for the camera or the sun.
- `editor/src/inspector.h`, the `duplicate` and `remove` field comment: not drawn for nothing
  selected or the camera's entity, and no Duplicate for the light's entity (0273).
- `editor/src/src.md`: no change unless its `inspector_buttons.c` line names Duplicate as
  where the picker is placed.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0;
`grep -n "duplicate" editor/src/inspector_buttons.c` shows no use of `inspector->duplicate`
in the swatch loop; `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints
`FINDINGS: 0`.

Human, in `examples/coin_game` (bug 01, steps 5–7 of `## How to test` in `feature.md`):
1. Select the sun by its marker; click the swatch of `colour`, then of `fill_colour`: each
   time the picker opens beside the Inspector, as it does for a shape's colour.
2. Pick orange for `colour`: the scene turns orange at once. Raise `fill_intensity` and pick a
   fill colour: the shadowed sides lighten in that colour.
3. Ctrl+Z undoes each change in turn.
4. Change a colour, Save, close, reopen: it is kept.
