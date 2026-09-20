# 19 — The Inspector knows the area its panel scrolls in
folder: editor
decisions: 0168, 0199, 0200

## Change
The open list has to be placed where it fits (ADR-0200), and the only rectangle that says how much room there is
round the button is the scroll area `dock.c` puts round a leaf's contents — the very thing that clips the list
(ADR-0199). The Inspector is drawn inside that area and never sees it. This card hands it over. Nothing reads it
yet and nothing behaves differently; card 21 is the reader.

`editor/src/inspector.h`:

- `voe_editor_inspector` gains one field, beside `content`:

		// The scroll area this panel's contents were drawn inside, as
		// dock.c handed it over this frame, and VOE_UI_NODE_NONE when
		// nobody did. voe_ui_node_visible of it is the room the open
		// list has to fit in: that area is what clips the list
		// (ADR-0199), so what is left of the area is exactly what can
		// be seen of anything drawn in it, however far it is scrolled.
		voe_ui_node area;

- and one call, declared after `voe_editor_inspector_frame_begin`:

		// Hands over the scroll area this panel's contents are about to
		// be drawn inside, for the open list to measure its room
		// against (ADR-0200). Called between voe_ui_frame_begin and
		// voe_editor_inspector_draw by whoever opened that area, which
		// is dock.c's walk; the node is this frame's, like every other
		// one on the struct, and a panel drawn inside nothing that
		// clips hands over VOE_UI_NODE_NONE.
		void voe_editor_inspector_area_set(voe_editor_inspector *inspector,
						   voe_ui_node area);

- The paragraph THE OPEN LIST IS DRAWN ON THIS PANEL AND NOT OVER IT gains one sentence: the area that clips the
  list is handed in by dock.c through the call above, because where the list fits is measured against that
  rectangle and this panel never sees the container it is drawn inside.

`editor/src/inspector.c`: the two-line setter, written beside `voe_editor_inspector_frame_begin`; and
`frame_begin` sets `area` to `VOE_UI_NODE_NONE` beside the `content` it already forgets, so a frame in which
nobody hands one over cannot read last frame's node. `frame_begin` runs before the dock's walk (interface.c),
which is what makes that order work.

`editor/src/dock.c`, in `walk_node`'s leaf branch and nowhere else: the `voe_ui_scroll_begin` call that wraps a
non-picture leaf keeps its node —

		voe_ui_node area = voe_ui_scroll_begin(...);

		// The Inspector's open list is drawn inside this area and
		// clipped by it, so this is the rectangle it fits itself into
		// (ADR-0200, inspector.h).
		if (node->panel == VOE_EDITOR_PANEL_INSPECTOR)
			voe_editor_inspector_area_set(&scene->inspector, area);

— before `voe_editor_panel_draw`. The walk already holds `scene`, and this file already includes `inspector.h`
for the panel's own draw.

## Done when
`checks.sh --folder editor` exits 0, `cmake --build --preset debug` builds the whole tree, and `voe_editor
--capture /tmp/editor.png --size 1280x720` (ADR-0177) still writes the same PNG of the editor it wrote before
this card — nothing this card adds draws anything. `grep -rn inspector_area_set editor` prints the declaration,
the definition and the one call in `dock.c`, and nothing else: no reader yet.
