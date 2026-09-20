# 08 — The selected Scene row is inverted
folder: editor
decisions: 0168, 0177, 0194, 0196

## Change
`editor/src/dock.c`: the Scene list's row becomes
`voe_ui_choice_begin(ui, "entity", i, voe_editor_scene_is_selected(scene, entities[i]))` and its label is a
plain `voe_ui_label(ui, rows[i].name)`; the comment above it says a selection is drawn inverted (ADR-0194)
and no longer mentions the accent. Nothing else changes: the node is still handed to
`voe_editor_scene_row_add` and read with `voe_ui_button_action`.

`editor/editor.md`: "The selected row in `Scene` is drawn in the theme's accent." becomes a sentence saying
it is drawn inverted, like anything held or pressed, and that the editor uses no colour to mean anything
(ADR-0194).

## Done when
The folder's check passes (`checks.sh` for `editor`), and `voe_editor --capture /tmp/scene.png --size
1280x720` (ADR-0177) writes a PNG in which the selected row is a light bar with dark text.
