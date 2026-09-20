# 14 — The open list follows its button and is picked from
folder: editor
decisions: 0168, 0190, 0193, 0195, 0198, 0199
read: bugs/01-the-open-list-does-not-follow-the-button-when-the-panel-scrolls.planned.md

## Change
`editor/src/inspector_edit.c`, all of it inside `voe_editor_inspector_buttons_read`, which already runs in the
one window a rectangle and a click answer in. In this order:

1. Before the Duplicate, Delete, Remove and Add component buttons: each row in `inspector->rows` whose
   `voe_ui_button_action` fired — at most one can — is
   `voe_editor_inspector_named_submit(inspector, scene->world, inspector->dropdown.entity,
   inspector->dropdown.type, inspector->dropdown.offset, row.value)` followed by
   `voe_editor_scene_dropdown_close(scene)`. The target is this frame's copy on the inspector and not
   `scene->dropdown`, for the reason `inspector->entity` is read instead of the live selection.
2. The block that opens the list on a fired dropdown control stays where it is, and stops reading a rectangle:
   `left` and `top` are handed in as nought, step 4 below being what puts the numbers in — on this very frame,
   so a list is drawn in the right place from the first frame it shows.
3. After the buttons, while `inspector->dropdown.open`: a primary-button press that went down this frame and
   fired neither one of those rows nor a dropdown control closes the list. It is the same press edge the Add
   component choices are hidden on, read the same way and before `inspector->pointer_was_down` is written at
   the end of the function. And a list that was open this frame with `inspector->row_count` nought is closed
   too: its field was not on the panel at all — nothing selected, another entity selected, or the component
   gone — which is what scene.c's deleted `_dropdown_showing` used to answer.
4. Last, while `scene->dropdown.open` — the live one, so that a list opened in step 2 is placed at once — and
   `inspector->content` is not `VOE_UI_NODE_NONE`: find the control among `inspector->controls` whose `names`
   is not NULL and whose `type` and `offset` are the open dropdown's, and with
   `voe_ui_rect b = voe_ui_node_rect(ui, control->node)` and `voe_ui_rect c = voe_ui_node_rect(ui,
   inspector->content)` call

		voe_editor_scene_dropdown_place(scene, b.min.x - c.min.x,
						b.min.y + b.size.y - c.min.y);

   the button's left edge and just under its bottom, in the content column's space. No such control is no
   placement and no assert. Both rectangles are moved by the same scroll offset, so the pair this works out is
   the same however far the panel is scrolled, and the list drawn from it next frame sits under its button
   wherever the button has gone.

`editor/src/inspector_edit.h`: the paragraph about a fired dropdown control gains what this file now does with
the list once it is open — a row that fired is submitted at once as the field's value and closes the list, a
press on neither the rows nor the control closes it, a frame in which the list drew no rows closes it, and
where it sits is measured from the button's rectangle and set through scene.h every frame it is open, because
an overlay is positioned from its widget each frame and never once when it opened (ADR-0199). Say that the
arithmetic is in the Inspector's content column's space and therefore says nothing about scrolling.

`editor/src/interface.h`: the budget paragraph THE OPEN DROPDOWN is rewritten for where the list now is. It is
`inspector.c`'s, drawn inside the Inspector's scroll area and clipped by it, so it never reaches over the rest
of the editor (ADR-0199). Its thirty-four nodes stand — the anchored column, one; its panel, one; and up to
`VOE_EDITOR_DROPDOWN_ROWS` (inspector.h, 16) rows, each a choice button and the label composed into it,
thirty-two — and the content column `inspector.c` opens round everything that panel draws is one more:
547 + 34 + 1 = 582. The four hundred and eighteen elements stand unchanged, a column drawing none of its own,
so the total stays 5656; `VOE_EDITOR_INTERFACE_NODES` becomes 582 and the other two are untouched, the list
still not scrolling.

## Done when
The coder: `checks.sh --folder editor` exits 0 and `cmake --build --preset debug` builds the whole tree.

The human, at a running `voe_editor` on a project with enough entities or components that the Inspector panel
scrolls: selects an entity with a Shape, opens the kind dropdown, and scrolls the panel up and down with the
wheel while it is open — the list stays glued under its button the whole way, and is cut off with the button at
the panel's edge instead of floating on over the editor. Picking Cylinder from it still changes the shape in
both views at once and marks the project unsaved; Escape and a click elsewhere still close it with the kind
unchanged.
