# 11 — A slider is a number box with a thumb
folder: ui
decisions: 0168, 0192, 0196

## Change
A new widget, built out of this folder's public calls only — no context record and no widget kind of its
own (ADR-0196).

`ui/include/ui/slider.h` (new): a header comment saying what it is (a horizontal track of a fixed width in
millimetres with a thumb at the value's place in `min..max`), that it IS a number box, so a drag across it
changes the value, a click opens it for typing and a drag while held draws it inverted (ADR-0196), that the
value is clamped to the range on the way out, and what it costs in nodes (three) and element records (the
box's one plus the thumb's two).

	voe_ui_node voe_ui_slider(voe_ui_context *ui, const char *name, uint32_t index,
				  double value, double min, double max, float width_mm);
	typedef struct { bool held; bool changed; double value; } voe_ui_slider_result;
	voe_ui_slider_result voe_ui_slider_action(const voe_ui_context *ui, voe_ui_node slider,
						  double min, double max);

`ui/src/slider.c` (new): `voe_ui_slider` opens `voe_ui_number_begin(ui, name, index, value,
(max - min) / width_mm)`, and inside it a row of fixed size `width_mm` × `VOE_UI_SLIDER_HEIGHT` (4 mm)
holding one anchored `voe_ui_panel_begin(..., VOE_UI_SURFACE_RAISED, ...)` of `VOE_UI_SLIDER_THUMB` (3 mm)
wide, filling the height, its anchor `x` offset `(float)((value - min) / (max - min)) * (width_mm -
VOE_UI_SLIDER_THUMB)` clamped to the track; closes both. `voe_ui_slider_action` calls
`voe_ui_number_action`, clamps `value` into `min..max`, and answers `held` and `changed` (false when the
clamp left the value where it came in). Read `ui/include/ui/layout.h`'s anchor paragraphs and
`ui/include/ui/widgets.h`'s number box paragraphs; read no other file.

`ui/tests/slider.c` (new): the thumb sits at the left edge at `min`, at the right edge at `max` and halfway
at the middle; a sideways drag past `VOE_UI_NUMBER_DEAD_ZONE` moves the value by the distance times
`(max - min) / width_mm`; a drag beyond either end answers exactly `min` or `max`; a frame with no pointer
on it answers the value handed in and `changed` false.

Add `include/ui/slider.h` to `ui/ui.md`'s list, `slider.c` to `ui/src/src.md` and `ui/tests/tests.md`.

## Done when
The folder's check passes (`checks.sh` for `ui`) with `ui/tests/slider.c` among the tests that ran.
