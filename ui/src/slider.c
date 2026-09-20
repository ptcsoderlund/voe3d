// The slider: a number box with a track and a thumb in it (ADR-0196). Built
// out of this folder's public calls and nothing else — no widget kind, no
// field in the context, no entry in context.h — so everything it does is what
// voe_ui_number_begin, voe_ui_row_begin and voe_ui_panel_begin already do. The
// header says why that is the whole design; this file is the arithmetic.
#include <ui/slider.h>

#include <ui/layout.h>
#include <ui/widgets.h>

// `x` into `low..high`, with anything that is not a number landing at `low`:
// the range may be empty and the value may have come from a division by a zero
// span, and `!(x > low)` catches a NaN where `x < low` would let it through.
static double clamped(double x, double low, double high)
{
	if (!(x > low))
		return low;
	return x > high ? high : x;
}

voe_ui_node voe_ui_slider(voe_ui_context *ui, const char *name, uint32_t index,
			  double value, double min, double max, float width_mm)
{
	// How far the thumb's left edge can travel: the track less the thumb,
	// so the thumb's far edge stops at the track's and does not hang off
	// the end at `max`.
	double travel = (double)width_mm - (double)VOE_UI_SLIDER_THUMB;
	// One track's width is one range, which is what makes the thumb follow
	// the pointer: drag the whole width and the value crosses the whole
	// range.
	voe_ui_node slider = voe_ui_number_begin(ui, name, index, value,
						 (max - min) / (double)width_mm);
	float at = (float)clamped((value - min) / (max - min) * travel, 0.0,
				  travel);

	// The track. A row rather than a panel because the number box already
	// draws the fill behind it, and fixed on both axes because an anchored
	// child contributes nothing to a parent's natural size — a track that
	// fitted its children would come out at nothing at all.
	voe_ui_row_begin(ui, (voe_ui_container){
				     .size = { { VOE_UI_SIZE_FIXED, width_mm },
					       { VOE_UI_SIZE_FIXED,
						 VOE_UI_SLIDER_HEIGHT } } });

	// The thumb, out of the track's run and pinned to it: fixed on X at
	// `at` from the left edge, filling the track on Y.
	voe_ui_panel_begin(
		ui, "thumb", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.size = { { VOE_UI_SIZE_FIXED, VOE_UI_SLIDER_THUMB },
				  { VOE_UI_SIZE_NATURAL, 0.0f } },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_START, at },
				    .y = { VOE_UI_ACROSS_FILL, 0.0f } } });

	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_end(ui);

	return slider;
}

voe_ui_slider_result voe_ui_slider_action(const voe_ui_context *ui,
					  voe_ui_node slider, double min,
					  double max)
{
	voe_ui_number_result number = voe_ui_number_action(ui, slider);
	voe_ui_slider_result result;

	result.held = number.held;
	result.value = clamped(number.value, min, max);
	// The clamp is a change in its own right, and it has to be: the number
	// box says nothing changed when it hands back the value it was given,
	// and a caller that writes back only on `changed` would then keep a
	// value from outside the range for ever.
	result.changed = number.changed || result.value != number.value;

	return result;
}
