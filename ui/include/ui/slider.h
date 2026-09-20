// The slider (ADR-0196): a horizontal track of a fixed width in millimetres
// with a thumb sitting at the value's place in `min..max`, for the places a
// range is worth seeing as well as reading — the two contrast scalars a theme
// is authored from, first of all.
//
//     voe_ui_node s = voe_ui_slider(ui, "contrast", 0, contrast,
//                                   VOE_UI_THEME_SCALAR_MIN,
//                                   VOE_UI_THEME_SCALAR_MAX, 40.0f);
//     ...
//     voe_ui_slider_result r = voe_ui_slider_action(
//             ui, s, VOE_UI_THEME_SCALAR_MIN, VOE_UI_THEME_SCALAR_MAX);
//     if (r.changed)
//             contrast = r.value;
//
// IT IS A NUMBER BOX, AND THAT IS THE WHOLE OF IT. There is no slider widget
// kind, no record of one in the context and no gesture of its own: the call
// below opens a voe_ui_number_begin with the range spread over the track's
// width, puts a track and a thumb inside it, and closes it again. So a drag
// across it changes the value at one width per range, a click inside the dead
// zone opens it for typing (ADR-0192), it is drawn inverted while it is
// dragged (ADR-0196), and Tab reaches it as it reaches any number box. Its own
// widget kind, like the colour picker's, was rejected as more state and more
// files for what a number box already does.
//
// THE VALUE IS CLAMPED TO THE RANGE ON THE WAY OUT AND NEVER ON THE WAY IN.
// A number box hands back the value it was given plus this frame's drag and
// knows no limits (widgets.h), so a hand that drags past the end would run the
// value away; voe_ui_slider_action is where the range is applied, and a value
// handed in from outside the range comes back clamped with `changed` set, so a
// caller that writes back what it is given ends up inside the range.
//
// IT IS PLACED WHEREVER THE CALL IS MADE, as any widget is, and takes the
// theme in force there: the track is the number box's own fill — control,
// control_hovered, `inverse` while it is dragged — and the thumb is a raised
// panel on it.
//
// WHAT IT COSTS, SO A CALLER CAN SIZE ITS CAPACITIES: three nodes — the number
// box, the track inside it and the thumb — and three element records, the
// box's one fill plus the thumb's two, a raised panel's border and surface.
// While it is open for typing the number box adds what a number box being
// typed into adds, and needs a font for it.
#pragma once

#include <ui/layout.h>

#include <stdbool.h>
#include <stdint.h>

// The track's height and the thumb's width, in millimetres. Fixed rather than
// given, because a slider whose proportions were the caller's would be a
// second layout to get right at every call site; the width is the one thing a
// caller does choose, since how much room a range deserves is the panel's
// business and how thick it is is not.
#define VOE_UI_SLIDER_HEIGHT 4.0f
#define VOE_UI_SLIDER_THUMB 3.0f

// A slider showing `value` in `min..max` over a track `width_mm` wide, keyed
// by `name` and `index` as any widget is. Needs a theme in force, as the
// number box and the panel it is built from do. The node handed back is the
// number box's: it is what voe_ui_slider_action reads, and there is nothing to
// close — the call opens and closes everything it makes.
voe_ui_node voe_ui_slider(voe_ui_context *ui, const char *name, uint32_t index,
			  double value, double min, double max,
			  float width_mm);

// What the pointer and the keyboard did to one slider, read after
// voe_ui_frame_end through the node the call handed back. `min` and `max` are
// the same range the call was given; handing in another is the caller's own
// arithmetic, not a second opinion this folder holds.
typedef struct {
	// It is the slider the pointer went down on and has not let go of —
	// the number box's `held`, true for the whole drag wherever the
	// pointer has got to, and what the caller reads to know the thumb is
	// moving under the hand.
	bool held;
	// The value below is not the one handed in: this frame's drag moved
	// it, a typed commit was taken, or the clamp below pulled a value from
	// outside the range back into it. False when nothing moved and the
	// clamp left the value where it came in.
	bool changed;
	// The value, always inside `min..max`. Equal to what was handed in
	// whenever `changed` is false, so a caller may write it back every
	// frame or only when it changed and get the same answer.
	double value;
} voe_ui_slider_result;

voe_ui_slider_result voe_ui_slider_action(const voe_ui_context *ui,
					  voe_ui_node slider, double min,
					  double max);
