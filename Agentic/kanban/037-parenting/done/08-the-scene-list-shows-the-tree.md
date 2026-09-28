# 08 — The Scene list shows the tree
folder: editor
decisions: 0168, 0281, 0194

## Change
Needs cards 06 and 07. Display only; dragging is card 09 (0281 point 7).

- `editor/src/scene_list.c`, `editor/src/scene_list.h`:
  - Rows are drawn depth-first: each authored root (an identity row with no parent row, or whose
    parent is dead or has no identity) in identity-table order, and right after each row the
    authored entities whose parent it is, in identity-table order, at any depth. Explicit stack,
    capped at `VOE_SCENE_PARENT_DEPTH_MAX` (`scene/parent_component.h`); an entity caught in a
    loop is still listed, after the rest, at depth 0, so no authored entity goes missing.
  - Each row is indented by its depth, a fixed number of millimetres per level (a named
    constant), made the way `ui/include/ui/layout.h` says a spacer is made. Keep each widget's
    key the identity-table index, so a row's highlight follows its entity when the order changes.
  - `voe_editor_scene_row_add` is still called once per row, in drawn order.
  - Record the "Scene" heading's node on the scene for card 09: new field
    `voe_ui_node heading` in `voe_editor_scene` (`editor/src/scene.h`), and
    `voe_editor_scene_rows_clear` in `editor/src/scene.c` sets it to `VOE_UI_NODE_NONE`.
  - Header points: the order and why a loop's entities still show; the indent.
- `editor/src/src.md`: the `scene_list` entries mention the tree.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0 and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. The human's: in a scene whose text gives one thing a
`[N.voe_scene_parent]` section, the list shows it indented under its parent.
