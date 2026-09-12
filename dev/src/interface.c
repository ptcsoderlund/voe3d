// The interface's content: a heading, two buttons and three number boxes on a
// semitransparent panel. See the header for why it is one draw command and why that is the
// point of it.
//
// THE TWO BUTTONS DO DIFFERENT THINGS ON PURPOSE. One counts and shows its
// count in its own label — "click: 0", "click: 1" — which is the proof that a
// click reached the program rather than merely changing a colour; the other does
// nothing at all and is there to be the button that is NOT hovered in every
// screenshot, so that "hovered looks different" is a comparison in one picture
// rather than a claim about two.
//
// AND THE COUNTING ONE GETS WIDER AT TEN, WHICH IS NOT A GLITCH. Its size is its
// label's measured size plus a button's padding, so the frame the count reaches
// two digits is the frame the button grows and the row beside it moves. That is
// layout doing what it is for — the number is measured from the font, nothing is
// reserved for a width nobody asked for — and it is the cheapest demonstration
// in the program that a label's size is measured rather than assumed.
//
// THE COUNT IS THIS FILE'S AND NOT `ui`'S, which is the shape of an immediate
// mode interface: the widget says a click happened and the program remembers
// what that means. `ui` keeps two ids and nothing else — no values, no per
// widget table — and a counter living here rather than there is what that
// decision looks like from the caller's side.
//
// ---- THE THREE NUMBER BOXES, AND WHY THEY DRIVE THE PANEL THEY SIT ON ----
//
// Drag one sideways and the value changes; hold the fine key and it changes a
// tenth as fast; click one without moving and nothing happens at all, which is
// the click reserved for the typing that does not exist yet.
//
// EACH OF THEM CHANGES THE PANEL IT IS STANDING ON, so there is nothing to look
// away at to see whether it worked: `alpha` fades the plate, `text` grows every
// letter on it, and `gap` pushes the rows apart. It also puts the widget through
// the case that is easiest to get wrong — a drag whose own box MOVES under the
// pointer while it is being dragged, because the thing it is changing is the
// layout around it. It keeps working, and that is `ui` keying a widget by its
// path rather than by where it is.
//
// THE VALUE IS THIS FILE'S, EXACTLY AS THE CLICK COUNT IS. `ui` is handed the
// value every frame and hands back what the drag made of it; the three doubles
// below are where it actually lives.
//
// AND THE CLAMPING AND THE ROUNDING ARE THIS FILE'S TOO, WHICH IS THE POINT OF
// THE THIRD ONE. `ui` knows no field kinds: it cannot know that an alpha stops
// at one, that a text scale must stay above nought, or that a gap is a whole
// number of millimetres. `gap` is the demonstration — the drag accumulates
// smoothly in a double and the layout is given the rounded value, so it steps
// cleanly without the gesture losing the fractions between steps.
//
// EVERY COLOUR IN HERE IS LINEAR, as everything crossing render's boundary is.
// There is exactly one of them, the panel's, because every other colour on the
// interface is `ui`'s own and card 036 is what replaces those.
#include "interface.h"
#include "surface.h"

#include <base/assert.h>

#include <math/float2.h>
#include <math/float4.h>
#include <ui/widgets.h>

#include <stdio.h>

// Where the panel sits: in from three edges, and far enough down the page to
// clear the readout, which owns the top-left corner and ends around forty.
//
// INTERFACE_TOP IS THE WHOLE DISTANCE FROM THE TOP EDGE AND NOT AN EXTRA ON TOP
// OF THE INSET. It was a 54 mm spacer sitting inside a 6 mm inset, so content
// began 60 mm down; now that it is the root's top padding it says 60 outright,
// because two numbers that have to be added at the call site is how a
// rearrangement quietly moves the interface.
#define INTERFACE_INSET 6.0f
#define INTERFACE_TOP 60.0f

// A dark plate, mostly see-through, so that what is behind the interface shows
// through it — which is the whole reason a panel has an alpha and is worth
// looking at in the picture.
#define PANEL_ALPHA 0.72f

// What one millimetre of sideways drag is worth on each of the three, and the
// range this program will let each of them have. The limits are here and not in
// `ui` because they are facts about what the value MEANS: an alpha above one is
// not a colour, a text scale of nought asserts inside `ui`, and a negative gap
// is not a gap.
//
// A HUNDRED MILLIMETRES ACROSS THE WHOLE RANGE, on all three, which at this
// surface's scale is a comfortable sweep of the hand — and a tenth of that with
// the fine key held.
#define ALPHA_PER_MM 0.01
#define ALPHA_LEAST 0.0
#define ALPHA_MOST 1.0

#define TEXT_PER_MM 0.01
#define TEXT_LEAST 0.6
#define TEXT_MOST 2.0

#define GAP_PER_MM 0.2
#define GAP_LEAST 0.0
#define GAP_MOST 20.0

static double clamped(double value, double least, double most)
{
	if (value < least)
		return least;
	if (value > most)
		return most;
	return value;
}

// The nearest whole number, for a value this program has already clamped to
// nought or more. Written out rather than taken from <math.h>, which this file
// would otherwise not need at all.
static float rounded(double value)
{
	return (float)(int)(value + 0.5);
}

voe_ui_context *voe_dev_interface_new(voe_base_arena *arena,
				      const voe_text_font *font)
{
	voe_ui_context *ui;

	VOE_BASE_ASSERT(arena != NULL, "making an interface without an arena");
	VOE_BASE_ASSERT(font != NULL, "an interface with no font");

	ui = voe_ui_context_new(
		arena, (voe_ui_capacities){
			       .nodes = VOE_DEV_INTERFACE_NODES,
			       .elements = VOE_DEV_INTERFACE_ELEMENTS });
	voe_ui_font_set(ui, font);

	return ui;
}

bool voe_dev_interface_draw(voe_render_device *gpu, voe_ui_context *ui,
			    voe_base_arena *arena, voe_platform_size target,
			    voe_platform_pointer pointer, bool down, bool fine,
			    uint32_t *elements)
{
	// How many times the counting button has been clicked, and the label
	// it puts on itself. Both live for the life of the program, which is
	// what an immediate-mode caller's own state looks like.
	static uint32_t clicks;
	static char counted[32];

	// And the three the number boxes drive, which live here for exactly the
	// same reason. `ui` is handed each of them every frame and remembers
	// none of them.
	static double alpha = PANEL_ALPHA;
	static double text = 1.0;
	static double gap = 3.0;
	static char alpha_label[32];
	static char text_label[32];
	static char gap_label[32];

	voe_math_float4 plate = { 0.02f, 0.03f, 0.05f, (float)alpha };
	// ADR-0104's formula, and it is the same one src/surface.c uses and out
	// of the same two constants — one surface, one scale, and the mouse
	// below divides by this very number.
	float pixels_per_millimetre = (float)target.height /
				      VOE_DEV_SURFACE_HIGH * VOE_DEV_UI_SCALE;
	voe_math_float2 millimetres;
	struct voe_base_arena_mark mark;
	voe_ui_node count_button;
	voe_ui_node alpha_number;
	voe_ui_node text_number;
	voe_ui_node gap_number;
	uint32_t first;
	uint32_t count;
	bool ok = true;

	VOE_BASE_ASSERT(gpu != NULL, "drawing the interface on no device");
	VOE_BASE_ASSERT(ui != NULL, "drawing no interface");

	if (target.height == 0 || target.width == 0)
		return true;

	millimetres = voe_render_element_surface_size(target,
						      pixels_per_millimetre);

	// The tree lives here and nowhere else: the arena goes back as soon as
	// the records have been read out of it, which is before this returns.
	mark = voe_base_arena_mark(arena);

	// Before the frame, because it is what every label in it is measured
	// with — see ui/widgets.h on the text scale.
	voe_ui_text_scale_set(ui, (float)text);

	voe_ui_frame_begin(ui, arena);
	// THE POINTER'S PIXELS BECOME THE SURFACE'S MILLIMETRES BY ONE
	// DIVISION AND THERE IS NOTHING ELSE TO IT. Both spaces run x right
	// and y down from a top-left corner, so no axis turns over anywhere on
	// this line. `over` is passed through as `platform` reports it: a
	// pointer that has left the window or been locked for mouse-look is
	// not pointing at anything, and `ui` ends the hover for both.
	voe_ui_pointer_set(ui, (voe_ui_pointer){
				       .at = { pointer.x / pixels_per_millimetre,
					       pointer.y /
						       pixels_per_millimetre },
				       .over = pointer.over,
				       .down = down,
				       .fine = fine });

	// The root is the whole surface, so the interface is laid out in the
	// same millimetres everything else on it is.
	voe_ui_column_begin(
		ui, (voe_ui_container){
			    .size = { .along = { VOE_UI_SIZE_FIXED,
						 millimetres.y },
				      .across = { VOE_UI_SIZE_FIXED,
						  millimetres.x } },
			    .pad = { INTERFACE_INSET, INTERFACE_TOP,
				     INTERFACE_INSET, INTERFACE_INSET } });

	voe_ui_panel_begin(ui, "hud", 0, plate,
			   (voe_ui_container){ .across = VOE_UI_ACROSS_START,
					       .gap = rounded(gap),
					       .pad = { 4.0f, 4.0f, 4.0f,
							4.0f } });
	voe_ui_label(ui, "Interface");

	voe_ui_row_begin(ui, (voe_ui_container){ .gap = 3.0f });

	count_button = voe_ui_button_begin(ui, "count", 0);
	snprintf(counted, sizeof counted, "click: %u", clicks);
	voe_ui_label(ui, counted);
	voe_ui_end(ui);

	voe_ui_button_begin(ui, "quiet", 0);
	voe_ui_label(ui, "Or this one");
	voe_ui_end(ui);

	voe_ui_end(ui);

	// THE THREE NUMBER BOXES. Each is handed the value this file is holding
	// and a rate, and each is labelled with that same value — the widget
	// formats nothing, so how many decimal places a number deserves is
	// decided here, where what it means is known.
	voe_ui_row_begin(ui, (voe_ui_container){ .gap = 3.0f });

	alpha_number = voe_ui_number_begin(ui, "alpha", 0, alpha,
					   ALPHA_PER_MM);
	snprintf(alpha_label, sizeof alpha_label, "alpha %.2f", alpha);
	voe_ui_label(ui, alpha_label);
	voe_ui_end(ui);

	text_number = voe_ui_number_begin(ui, "text", 0, text, TEXT_PER_MM);
	snprintf(text_label, sizeof text_label, "text %.2f", text);
	voe_ui_label(ui, text_label);
	voe_ui_end(ui);

	// Shown as the whole number the layout is actually given, not as the
	// double behind it — otherwise the label would creep between steps
	// while the gap stood still, and the widget would look broken.
	gap_number = voe_ui_number_begin(ui, "gap", 0, gap, GAP_PER_MM);
	snprintf(gap_label, sizeof gap_label, "gap %d", (int)rounded(gap));
	voe_ui_label(ui, gap_label);
	voe_ui_end(ui);

	voe_ui_end(ui);

	voe_ui_end(ui);
	voe_ui_end(ui);

	if (!voe_ui_frame_end(ui)) {
		voe_base_arena_rewind(arena, mark);
		return false;
	}

	// The label above already says the old count, so the number a person
	// sees changes on the NEXT frame. That is immediate mode and not a lag
	// worth designing away: the click and the redraw are a sixtieth of a
	// second apart.
	if (voe_ui_button_action(ui, count_button).fired)
		clicks++;

	// EACH VALUE IS WRITTEN BACK WHOLE AND NEVER ACCUMULATED HERE. What
	// comes out of a number box is the value it was handed plus this
	// frame's drag, so this file stores it rather than adding anything to
	// it — which is the contract that lets typing into one, when it
	// arrives, land on these very lines unchanged.
	//
	// AND EACH IS CLAMPED ON THE WAY IN, because `ui` does not know what any
	// of them mean. A text scale that reached nought would assert inside
	// voe_ui_text_scale_set on the next frame.
	alpha = clamped(voe_ui_number_action(ui, alpha_number).value,
			ALPHA_LEAST, ALPHA_MOST);
	text = clamped(voe_ui_number_action(ui, text_number).value, TEXT_LEAST,
		       TEXT_MOST);
	gap = clamped(voe_ui_number_action(ui, gap_number).value, GAP_LEAST,
		      GAP_MOST);

	// The range this interface fills, taken before anything is submitted:
	// the panels and the screen-filling surface have already put theirs
	// into the same buffer this frame.
	first = voe_render_frame_elements_submitted(gpu);
	count = voe_ui_element_count(ui);
	for (uint32_t i = 0; i < count && ok; i++)
		ok = voe_render_frame_submit_element(gpu,
						     voe_ui_element(ui, i));

	voe_base_arena_rewind(arena, mark);
	*elements = count;

	if (!ok)
		return false;

	// ONE COMMAND, WHATEVER IS ON IT. Not one per widget and not one per
	// letter: every record in the range above is the same eighty bytes in
	// the same buffer, so a panel, two buttons and three strings go out
	// together.
	return voe_render_frame_draw_elements(
		gpu, voe_render_element_transform(millimetres), first, count);
}
