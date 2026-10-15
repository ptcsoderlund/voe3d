# 26 — The Inspector switches an entity off
folder: editor
after: 20, 23, 24, 25
decisions: 0168, 0427, 0428

## Change
0427's switch in the editor, 0428 point 9. Every system already honours
the identity's `off` (cards 19–25); this card only sets it and shows it.

`editor/src/scene.h` / `editor/src/scene.c`:
- `fold_flip` (scene.c, about line 164) becomes one static that submits the
  entity's identity row whole with one chosen bit flipped (`folded` or
  `off`), one structural change, or `full` when refused; the fold's caller
  passes `folded`.
- New `void voe_editor_scene_switch(voe_editor_scene *scene);` declared
  beside `voe_editor_scene_duplicate`: flips the selected entity's `off`
  through that static. Nothing selected, a prefab's part (0283 point 5) or
  the camera's entity does nothing, as `_duplicate` says for them. Its
  comment says so and that it is one undo step and unsaved. scene.h's
  header is at its cap of 60: the why goes above the declaration only.

`editor/src/inspector.h`: `voe_editor_inspector` gains `voe_ui_node enabled`
beside `duplicate`, commented as theirs are.

`editor/src/inspector.c`: the panel's first row, above the Duplicate and
Delete row and under the same condition (not a part, not the camera's
entity): a row of the label "Enabled" and a button labelled "On" or "Off"
by the selected identity's own `off`. Reset to `VOE_UI_NODE_NONE` where
`duplicate` is (line 331). The comment on the Duplicate/Delete condition
names the switch too. The identity section goes on listing `off` as a
toggle with the other fields, as it does `folded`.

`editor/src/inspector_buttons.c`: beside the duplicate check (line 138),
a fired `enabled` calls `voe_editor_scene_switch(scene)`.

`editor/src/scene_list.c`, `row_draw` (about line 110): a row is drawn
under `list_dim` also when `voe_scene_identity_off` is true of its entity,
so a child under an off parent is greyed as well; still selectable and
draggable. Its comment says so.

`editor/src/interface.h`: a budget paragraph after the Level section's, in
its style: the Enabled row adds four nodes (the row, the label, the
button, its label) and twelve elements ("Enabled" seven, the button's
border and fill two, "Off" three). `VOE_EDITOR_INTERFACE_NODES` 2559 → 2563,
`VOE_EDITOR_INTERFACE_ELEMENTS` 51496 → 51508. No scroll area: five stands.

`editor/src/src.md`: the entries for `scene.c`, `scene_list.c`,
`inspector.c` and `inspector_buttons.c` name the switch, its greyed row and
its button, each entry one sentence under 300 characters.

## Done when
`voe_editor` builds in the folder's check with no warning. The human runs
bug 01's How to test, steps 1–7
(`Agentic/kanban/089-a-model-has-lod-levels/bugs/01-i-cannot-switch-the-landscape-off-to-test-the-levels.planned.md`),
and every step holds; in particular:
- step 2: with both suns switched the other way, only the on one lights and
  shades; with both off the scene is dark;
- step 4: a click where the off terrain was selects what is behind it;
- step 6: after reopening, the terrain's Inspector says "Off", and in Play
  nothing stands or stops on it.
