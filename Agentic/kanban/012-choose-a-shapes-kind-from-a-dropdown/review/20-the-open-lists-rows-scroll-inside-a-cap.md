# 20 — The open list's rows scroll inside a cap
folder: editor
decisions: 0168, 0194, 0195, 0196, 0198, 0199, 0200

## Change
An overlay that fits on neither side of its button opens on the roomier one, capped to that room, and scrolls
inside itself (ADR-0200). This card gives the list the cap and the scrolling; nobody sets a cap yet, so the list
draws exactly as it does today. Card 21 is what puts a number in.

`editor/src/inspector.h`:

- `voe_editor_dropdown` gains one field, after `top`:

		// How tall the list's rows may be, in millimetres, or nought
		// for as tall as they come. Set only when the list fits neither
		// below the button nor above it: it is then capped to the room
		// on the roomier side and scrolls inside itself, so the last
		// value is still reachable (ADR-0200, inspector_edit.h). It is
		// the rows' own height and not the panel's — the panel is that
		// much plus its own padding and border.
		float height;

- `voe_editor_inspector` gains two, beside `rows`:

		// The open list's panel and the area its rows sit in, as drawn
		// this frame, VOE_UI_NODE_NONE when no list was drawn. Kept for
		// the read, which measures the rows against the room the panel
		// leaves (inspector_edit.h) and asks whether a press landed
		// inside the outline.
		voe_ui_node list;
		voe_ui_node list_rows;

- The paragraph THE OPEN LIST IS DRAWN ON THIS PANEL AND NOT OVER IT gains: that the rows sit in a scroll area of
  their own inside the list's panel, at their natural height while `height` is nought and capped to it when it is
  not, so the wheel over a capped list moves the rows within it and the list keeps its size and its place
  (ADR-0200); and that what the rows wanted is read back from that area with `voe_ui_node_measured` even while it
  is capped (ui/layout.h), which is what lets the read decide whether they would have fitted. And what the rows
  cannot take of a wheel gesture passes outward to the panel's own area behind them, as it does between any two
  nested areas (ui/widgets.h): the panel scrolls, the button moves, and the list follows it — which is why that
  is not a hole in ADR-0200 but the same rule twice.

`editor/src/inspector.c`, in the static function card 12 added and nowhere else:

- One constant beside `LIST_PAD`: `LIST_BAR 3.5f`, the thickness a Y scrollbar lies over the content with — the
  right padding `ui`'s own example gives such an area (ui/widgets.h, THE SCROLL AREA) — kept off the rows only
  while the list is capped, since that is the only time a bar shows.
- The panel keeps its `pad` and its `blocks_pointer`, and its `gap` goes to nought: it holds one child now. Its
  node is kept — `inspector->list = voe_ui_panel_begin(...)`.
- Between the panel and the rows, one area holding all of them:

		inspector->list_rows = voe_ui_scroll_begin(
			ui, "kinds", 0,
			(voe_ui_container){
				.size = { .along = capped
						  ? (voe_ui_size){ VOE_UI_SIZE_FIXED, h }
						  : (voe_ui_size){ 0 } },
				.across = VOE_UI_ACROSS_FILL,
				.gap = LIST_PAD,
				.pad = { .right = capped ? LIST_BAR : 0.0f } },
			(voe_ui_scroll_axes){ .y = true });

  with `h` the dropdown's `height` and `capped` that it is greater than nought. `size.along` is the height here,
  the panel being a column (ui/layout.h), and a NATURAL one is the rows as tall as they come, which is today.
  The rows and their labels are unchanged and go inside it, and its `voe_ui_end` comes before the panel's.
- The area is keyed `"kinds"`, which is claimed as a panel's key is and so is distinct from the panel's `"list"`.
  An area that is not called is forgotten (ui/widgets.h), so a list that closes and opens again starts at the top,
  which is what a re-opened list should do.

`voe_editor_inspector_frame_begin`, in that same file, sets `list` and `list_rows` to `VOE_UI_NODE_NONE` beside
the `content` it already forgets.

`editor/src/interface.h`: the budget paragraph THE OPEN DROPDOWN gains the area, in the voice of the rest. ONE
MORE NODE, the area its rows now sit in: 582 + 1 = 583. AND TWO MORE ELEMENTS, a track and a thumb on Y alone,
which is what the browser's own area is already counted at in this file: 5656 + 2 = 5658. AND ONE MORE SCROLL
AREA, on top of the dock's two and the browser's one, counted on top all the same for the reason the picker's
nodes are: `VOE_EDITOR_INTERFACE_SCROLLS` becomes 4. `VOE_EDITOR_INTERFACE_NODES` becomes 583 and
`VOE_EDITOR_INTERFACE_ELEMENTS` 5658.

## Done when
`checks.sh --folder editor` exits 0, `cmake --build --preset debug` builds the whole tree, and `voe_editor
--capture /tmp/editor.png --size 1280x720` still writes the same PNG it wrote before this card.

The human, at a running `voe_editor`: selects an entity with a Shape and opens the kind dropdown somewhere with
room below it. It looks exactly as it did — three rows, no bar, the same size and place — because no cap is set
until card 21.
