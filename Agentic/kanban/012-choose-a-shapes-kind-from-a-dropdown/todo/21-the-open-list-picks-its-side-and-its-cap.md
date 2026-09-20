# 21 — The open list picks its side and its cap
folder: editor
decisions: 0168, 0195, 0198, 0199, 0200
read: bugs/03-the-open-list-runs-off-the-bottom-and-cannot-be-reached.planned.md

## Change
The fit, decided every frame the list is open (ADR-0200), from the room the Inspector's scroll area leaves round
the button. The arithmetic belongs where the button's rectangle is already read — step 4 of
`voe_editor_inspector_buttons_read`, card 14 — and what it works out is written through the one call that moves
the list.

`editor/src/scene.h`: `voe_editor_scene_dropdown_place` gains a third number:

		void voe_editor_scene_dropdown_place(voe_editor_scene *scene,
						     float left, float top,
						     float height);

Its comment gains that `height` is how tall the rows may be, nought for as tall as they come, and that all three
are worked out afresh by inspector_edit.c every frame the list is open, because an overlay is placed where it
fits and the fit changes as the panel scrolls (ADR-0200). The paragraph THE OPEN DROPDOWN'S TARGET IS HERE TOO
gains the same in prose: the list opens below its button when it fits there, above it when it does not but fits
there, and on the roomier side capped and scrolling when it fits neither — so it is never drawn with values that
cannot be reached, and a list that opened downward flips when the panel scrolls its button toward the bottom
edge.

`editor/src/scene.c`: `_dropdown_place` writes `height` beside `left` and `top`, and still does nothing while the
list is closed.

`editor/src/inspector_edit.c`, inside `voe_editor_inspector_buttons_read`, in the step that places the list and
nowhere else. It already has the control it found, `voe_ui_rect b` for that button and `voe_ui_rect c` for
`inspector->content`. Keeping those two, and defaulting to today's answer — `top = b.min.y + b.size.y` and a
cap of nought, which is what a list that was not drawn this frame gets, the frame the button fired on:

		if (inspector->list != VOE_UI_NODE_NONE &&
		    inspector->area != VOE_UI_NODE_NONE) {
			voe_ui_rect w = voe_ui_node_visible(ui, inspector->area);
			voe_ui_rect p = voe_ui_node_rect(ui, inspector->list);
			voe_ui_rect r = voe_ui_node_rect(ui, inspector->list_rows);
			// The panel's own padding and border round its rows,
			// whether or not the rows are capped.
			float chrome = p.size.y - r.size.y;
			// What the whole list would be, uncapped: what the rows
			// wanted, which voe_ui_node_measured reports even while
			// they are capped (ui/layout.h).
			float want = chrome +
				     voe_ui_node_measured(ui, inspector->list_rows).y;
			float below = w.min.y + w.size.y - (b.min.y + b.size.y);
			float above = b.min.y - w.min.y;

			if (want <= below) {
				// below, as it is today
			} else if (want <= above) {
				top = b.min.y - want;
			} else {
				float room = below >= above ? below : above;

				cap = room - chrome;
				// A panel with almost no room either way still
				// shows a value to pick and scroll from.
				if (cap < b.size.y)
					cap = b.size.y;
				if (below < above)
					top = b.min.y - (cap + chrome);
			}
		}

then `voe_editor_scene_dropdown_place(scene, b.min.x - c.min.x, top - c.min.y, cap)` — `left` and `top` in the
content column's space exactly as card 14 left them, so scrolling still changes neither.

Every rectangle here is this frame's and read in the one window in which `ui` answers. `w` is what is left of the
Inspector's scroll area, so it is the room the list can be seen in however far the panel has scrolled; the three
nodes are `VOE_UI_NODE_NONE` on a frame that drew no list, which is the guard above and not an assert.

## Done when
The coder: `checks.sh --folder editor` exits 0 and `cmake --build --preset debug` builds the whole tree.

The human, at a running `voe_editor` on a project with enough entities or components that the Inspector panel
scrolls — the bug's own steps: scrolls the Inspector so an entity's Shape section sits near the very bottom of
the panel, with a row or two of room under the kind's button, and opens the list. It opens above the button
instead, whole, with Cylinder on it and pickable, and picking it changes the shape. Then, with the list open,
scrolls the panel slowly with the wheel: the list flips between below and above as the button moves and is never
left cut off at the panel's edge. Then makes the Inspector panel short enough — by dragging the window small —
that the list fits on neither side: it opens on the roomier side at that size and the wheel over it moves the
rows inside it, the last one included. Dragging the list's own scrollbar closes it for now; card 22 is that.
