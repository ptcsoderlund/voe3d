// A swatch and a colour picker (ADR-0192): two widgets any panel can use, both
// taking and handing back a linear RGB colour, the space an element record and
// a material are already in.
//
//     voe_ui_node s = voe_ui_button_begin(ui, "tint", 0);
//     voe_ui_swatch(ui, tint, (voe_ui_sizing){
//             .along = { VOE_UI_SIZE_FIXED, 6 },
//             .across = { VOE_UI_SIZE_FIXED, 6 } });
//     voe_ui_end(ui);
//     voe_ui_node p = picking ? voe_ui_colour_picker(ui, "tint", 0, tint)
//                             : VOE_UI_NODE_NONE;
//     ...
//     if (voe_ui_button_action(ui, s).fired)
//             picking = true;
//     if (picking) {
//             voe_ui_colour_result r = voe_ui_colour_picker_action(ui, p);
//             if (r.changed)
//                     tint = r.value;
//             if (r.outside)
//                     picking = false;
//     }
//
// THE PICKER IS A PANEL THE CALLER PLACES, NOT A POPUP THIS FOLDER OPENS.
// Wherever the call is made is where it sits — anchored beside a swatch, in a
// row of its own, anywhere a panel can go. A popup would have to escape every
// clip around its opener, and there is no such thing here: the Inspector's
// scroll area would cut off a popup anchored to a swatch inside it. So `ui`
// reports `outside`, a press that began off the picker, and whether that or
// Escape closes it is the caller's to decide.
//
// THE HUE SURVIVES A GREY. A grey has no hue, so a colour dragged to the
// square's left edge would lose it and snap to red. The context remembers the
// HSV each picker last showed under its key, for as long as the picker is made
// every frame, and uses it while the caller hands back the colour it produced;
// a colour from anywhere else is read afresh, keeping the remembered hue when
// that colour is a grey.
//
// THE GRADIENTS ARE CELLS, BECAUSE AN ELEMENT RECORD IS A SOLID RECTANGLE OR A
// GLYPH. The square is 20 x 20 solid cells, each the colour at its centre, and
// the strip 36: no texture, which would need an upload at startup this folder
// cannot make, and no shader change.
//
// WHAT IT COSTS, SO A CALLER CAN SIZE ITS CAPACITIES: six nodes (the panel,
// square, strip, field, the field's label and "not #RRGGBB"), and at most 461
// element records — 2 for the panel's border and surface, 400 cells and 2 for
// the square's marker, 36 cells and 1 for the strip's, 10 for the hex field
// and its text, caret and selection while it holds seven characters — one more
// for each typed past them — and 10 for "not #RRGGBB". A swatch is one
// node and one record. At most VOE_UI_COLOUR_PICKERS pickers in a frame (see
// ui/src/context.h), eight; one more refuses the frame and is named on stderr.
#pragma once

#include <ui/layout.h>

#include <math/float3.h>

#include <stdbool.h>
#include <stdint.h>

// THE SWATCH IS ONE SOLID ELEMENT AND ANSWERS NOTHING. A caller that wants to
// click it puts it inside a button, which is how a button composes anything.
//
// One solid element of `linear`, alpha 1, sized as a box with no content is:
// by `sizing`, so a NATURAL axis is nought. No identity, no theme, no answer
// to the pointer. VOE_UI_NODE_NONE when the frame has no room for a node.
voe_ui_node voe_ui_swatch(voe_ui_context *ui, voe_math_float3 linear,
			  voe_ui_sizing sizing);

// WHAT IT SHOWS: a 32 x 32 mm saturation/value square, value up and saturation
// right; under it a 32 x 4 mm hue strip; a marker on each at the current
// values; under those a hex field — card 09's field — reading `#RRGGBB`, and,
// after a refused commit, "not #RRGGBB" until the field is next edited. Hue,
// saturation and value are HSV of the sRGB colour, which is how a person reads
// a colour off any other picker. A press in the square or the strip, and a
// drag begun there wherever it goes, sets saturation and value or hue every
// frame. The hex field takes `#RRGGBB` or `RRGGBB` in any case on a commit —
// Enter, Tab or a press elsewhere — and refuses anything else, the colour
// staying as it was.
//
// The picker, keyed by `name` and `index` as a panel is, showing `linear`.
// Needs a font and a theme, as its field does. What it did is read after
// voe_ui_frame_end through voe_ui_colour_picker_action.
voe_ui_node voe_ui_colour_picker(voe_ui_context *ui, const char *name,
				 uint32_t index, voe_math_float3 linear);

typedef struct {
	// This frame's press or drag, or an accepted hex commit, moved the
	// colour.
	bool changed;
	// The colour, linear: the new one when `changed`, and otherwise the one
	// handed in, so a caller may write it back every frame.
	voe_math_float3 value;
	// The primary button went down this frame outside the picker's
	// rectangle.
	bool outside;
	// This frame's hex commit was not `#RRGGBB` or `RRGGBB`; the colour did
	// not move.
	bool refused;
} voe_ui_colour_result;

// Reading before the frame has ended, or through a node that is not a picker,
// is the caller's bug and asserts. A refused frame answers with the colour
// handed in and nothing else.
voe_ui_colour_result voe_ui_colour_picker_action(const voe_ui_context *ui,
						 voe_ui_node picker);
