# 03 — A folding section in ui
folder: ui
after: 02
decisions: 0168, 0496, 0503

## Change
Build the one folding section any panel can use (0496 point 1, 0503 points 1 and 2).

- `ui/include/ui/widgets.h`: add a part for the section after the choice. It holds:
  - `typedef struct { voe_ui_node box; voe_ui_node fold; } voe_ui_section;`
  - `voe_ui_section voe_ui_section_begin(voe_ui_context *ui, const char *name, uint32_t index, bool open);`

  The comment carries a usage sketch and makes these points:
  - The call opens the box and, inside it, the header row, with the fold button first. The button
    reads `-` when open and `+` when folded.
  - The caller composes the title into the header row: a HEADING label, or a choice holding one.
    It closes the row with `voe_ui_end`, puts the body's rows in only while open, and closes the
    box with `voe_ui_end`.
  - The caller holds whether the section is open, and flips that when
    `voe_ui_button_action(ui, section.fold).fired`. Nothing here remembers a fold, so the caller
    saves it wherever it saves things.
  - The box is a panel on `surface` with its hairline border. It grows to its parent's width and
    fits its content's height. Its pad and gap are scaled by the theme's `spacing`, as a button's
    are. So where one section ends and the next begins is plain.
  - Sections nest.
  - The box is keyed as a panel is, by name and index. The fold button is keyed inside it.
  - What it costs: the box, row, button and button label are four nodes, and they make five
    element records (the box 2, the button 2, the label 1).
- New `ui/src/section.c`, with a header comment: the function, built from `voe_ui_panel_begin`,
  `voe_ui_row_begin` (layout.h), `voe_ui_button_begin` and `voe_ui_label`. Nothing is added to
  `context.h`.
- New `ui/tests/section.c`, getting its font the way `ui/tests/widgets.c`'s label case does. Cases:
  - An open section: the header row is above the body rows, and both are inside the box.
  - A folded section with no body: the box is the header row's height plus its pad.
  - A press and release on `fold` fires its action. A press and release on a title choice fires
    only the choice.
  - A section in another's body lies inside the outer box.
  - One name with two indexes makes two sections, and the frame is not refused.
- `ui/src/src.md`, `ui/tests/tests.md`, `ui/ui.md`, `ui/include/ui/ui.md`: entries for the section.

## Done when
The test `ui/section` passes.
