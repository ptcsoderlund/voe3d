# 08 — A fired dropdown opens the list, and a choice is an intent
folder: editor
decisions: 0168, 0190, 0193, 0195, 0198

## Change
`editor/src/inspector_edit.c`:

- In `voe_editor_inspector_buttons_read`, beside the block that opens the colour picker on a fired swatch: a
  control whose `names` is set and whose `voe_ui_button_action` fired opens the list instead, with
  `voe_editor_scene_dropdown_open` on a `voe_editor_dropdown` of `inspector->entity`, the control's `type` and
  `offset`, its `names`, and `left` and `top` taken from `voe_ui_node_rect(ui, control->node)` — the rectangle's
  left edge and its bottom, so the list hangs under the button that opened it. The rectangle is read in this same
  window, which is the only one in which `ui` answers (ui/layout.h).
- In `voe_editor_inspector_edits_read`, a control whose `names` is set is skipped, the way a colour's control is
  skipped: it is a button and not a number box, and `voe_ui_number_action` on a button asserts.
- A new function beside `voe_editor_inspector_colour_submit` and written the same way:

		void voe_editor_inspector_named_submit(
			voe_editor_inspector *inspector, voe_ecs_world *world,
			voe_ecs_entity entity, voe_ecs_type type, size_t offset,
			uint32_t value);

  the row read, copied into a zeroed intent whose entity sits at offset zero, the four bytes at `offset`
  overwritten with `value`, the intent submitted and one counted in `replaced` — so the project is marked
  unsaved by the count `main.c` already reads, and the owning system is what actually changes the row (ADR-0134
  point 4, 0190). An entity no longer alive or without the row submits and counts nothing.

`editor/src/inspector_edit.h`: declare `voe_editor_inspector_named_submit`, with the comment saying it is the
open list's choice, read by interface.c in the same window as the edits, and that what it submits is the index
into the field's names because entry `i` names value `i` (0198). Its prose gains one sentence saying a fired
dropdown control opens the list through scene.h and writes nothing itself.

## Done when
`checks.sh` for `editor` exits 0 and `cmake --build --preset debug` builds the whole tree. The list is drawn by
card 09; until then a fired dropdown sets the target and nothing shows.
