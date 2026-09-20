# 12 — The open list is drawn on the panel and not over it
folder: editor
decisions: 0168, 0194, 0195, 0196, 0198, 0199

## Change
The list moves out of the root's frame and into the Inspector panel, where the button that opened it is. It is
an anchored child of one column this panel opens round everything it draws, so `ui` scrolls it with that
column's content and the scroll area clips it, exactly as it does every other thing on the panel (ui/layout.h:
an anchored child is clipped like any other, and a scroll offset moves anchored children with the rest).

`editor/src/inspector.h`:

- `voe_editor_dropdown`'s `left` and `top` are re-spelled — the fields stay, their space changes:

		// Where the list's top-left corner goes, in millimetres from
		// the top-left of this panel's content column (`content`
		// below) and not from the surface. The column and the button
		// the list hangs from are moved by the same scroll offset, so
		// this pair does not change as the panel scrolls; what it is
		// measured from is the button's rectangle, every frame the
		// list is open (ADR-0199, inspector_edit.h).
		float left;
		float top;

- one more type beside `voe_editor_inspector_type_button`:

		// One row of the open list as this panel drew it: the choice
		// button, and the value it names.
		typedef struct {
			voe_ui_node node;
			uint32_t value;
		} voe_editor_dropdown_row;

- `voe_editor_inspector` gains three things, each commented in the voice of the fields above it:
  `voe_editor_dropdown dropdown;` — what the open list is open on, as it stood when this frame began, copied out
  of scene.h's by `voe_editor_inspector_frame_begin` so that the panel draws from one value all frame and the
  read that follows sees the same one; zeroed is a closed list. `voe_ui_node content;` — the one column
  everything this panel draws sits in and the thing the open list is anchored to, `VOE_UI_NODE_NONE` when
  nothing was drawn. `voe_editor_dropdown_row rows[VOE_EDITOR_DROPDOWN_ROWS]; uint32_t row_count;` — the open
  list's rows as drawn this frame, kept for the read for the reason every other control is.
- `voe_editor_inspector_frame_begin` takes the target as well:

		void voe_editor_inspector_frame_begin(voe_editor_inspector *inspector,
						      voe_base_arena *arena,
						      const voe_editor_dropdown *dropdown);

  saying NULL is a closed one.
- One new prose paragraph, after A NAMED FIELD IS A DROPDOWN: THE OPEN LIST IS DRAWN ON THIS PANEL AND NOT OVER
  IT (ADR-0199). It says that an overlay belongs to the widget it opened from, so the list is an anchored child
  of this panel's own content column and is scrolled and clipped with it — when the button scrolls out of the
  panel the list goes with it instead of floating over the editor; that it is emitted after every section
  because submission order is paint order (ui/layout.h) and a list emitted beside its button would be painted
  over by the rows below it; and that its offset is in that column's space, so that scrolling changes neither
  of the two numbers.

`editor/src/inspector.c`:

- Two constants beside `COMPONENT_GAP`: `LIST_PAD 1.0f`, round the open list's rows and between them — the
  number `interface.c`'s `DROPDOWN_PAD` has today, moved with the block below; and `CONTENT_GAP 2.0f`, the gap
  `dock.c`'s scroll area gives a panel's children (its `PANEL_GAP`) — read that file, change nothing in it —
  because the content column is now the one child that area holds and the space between the sections is this
  column's to declare.
- `voe_editor_inspector_frame_begin` copies `*dropdown` into `inspector->dropdown` (a zeroed one for NULL), sets
  `row_count` to nought and `content` to `VOE_UI_NODE_NONE`, beside what it already forgets.
- `voe_editor_inspector_draw`, after the aliveness check that draws NOTHING_TEXT and returns: open the content
  column round the Duplicate and Delete row, the walk and `add_component` —
  `inspector->content = voe_ui_column_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_FILL, .gap = CONTENT_GAP });`
  — then the open list, then one `voe_ui_end(ui)`. The panel looks exactly as it does today; the capture below
  is the proof.
- One static function above it drawing the list, which returns at once unless `inspector->dropdown.open`, its
  `names` is not NULL, its `entity` is this frame's `inspector->entity` and `voe_ecs_component_get(world,
  dropdown.type, entity)` hands back a row. The value in force is the `uint32_t` copied out of that row at
  `dropdown.offset`, the way scene.c's `voe_editor_scene_dropdown_showing` copies it. What it then draws is
  `interface.c`'s list block, moved as it stands rather than written again — the anchored column, now

		voe_ui_column_begin(ui, (voe_ui_container){ .anchor = {
			.anchored = true,
			.x = { VOE_UI_ACROSS_START, inspector->dropdown.left },
			.y = { VOE_UI_ACROSS_START, inspector->dropdown.top } } });

  its one panel with `LIST_PAD` as gap and padding, and one `voe_ui_choice_begin(ui, "kind", i, i == value)`
  with the name as a plain `voe_ui_label` inside it for every `i` below `names->value_count` whose entry is not
  NULL, at most `VOE_EDITOR_DROPDOWN_ROWS` — so the value in force is still marked by inversion and nothing else
  (ADR-0194, ADR-0196). Each row's node and the value it names go into `inspector->rows`.

`editor/src/interface.c`: it stops drawing and reading the list. `voe_editor_inspector_frame_begin` is called
with `&scene->dropdown`. `DROPDOWN_PAD`, `struct dropdown_row`, the locals that held whether it showed, its
value, the captured target and the rows, the `voe_editor_scene_dropdown_showing` call, the block that drew the
anchored list and the block after `voe_ui_frame_end` that read its rows and closed it on an outside press all
go. What stays is the Escape close, beside the picker's. Its header paragraph AND THE OPEN DROPDOWN IS DRAWN AND
READ HERE is replaced by one saying the open list is the Inspector's own (inspector.h): it is drawn inside that
panel so that it moves and disappears with the button it hangs from (ADR-0199), and the only thing this file
still does to it is close it on Escape.

## Done when
`checks.sh --folder editor` exits 0, `cmake --build --preset debug` builds the whole tree, and `voe_editor
--capture /tmp/editor.png --size 1280x720` still writes a PNG of the editor that looks as it did before this
card — nothing is selected in a captured frame, so the content column is the only thing that could have moved,
and nothing may have.

One list is drawn now instead of two, and it is drawn in the wrong place and cannot be picked from: `left` and
`top` still hold the surface coordinates an older card wrote there, and the rows are recorded and read by
nobody. Card 13 and card 14 are what put the right numbers in and read the rows.
