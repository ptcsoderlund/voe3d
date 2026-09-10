// The interface's content: a heading and two buttons on a semitransparent
// panel. See the header for why it is one draw command and why that is the
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

// Where the panel sits: in from the left edge, and far enough down the page to
// clear the readout, which owns the top-left corner and ends around forty.
#define INTERFACE_INSET 6.0f
#define INTERFACE_TOP 54.0f

// A dark plate, mostly see-through, so that what is behind the interface shows
// through it — which is the whole reason a panel has an alpha and is worth
// looking at in the picture.
#define PANEL_ALPHA 0.72f

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
			    voe_platform_pointer pointer, bool down,
			    uint32_t *elements)
{
	// How many times the counting button has been clicked, and the label
	// it puts on itself. Both live for the life of the program, which is
	// what an immediate-mode caller's own state looks like.
	static uint32_t clicks;
	static char counted[32];

	voe_math_float4 plate = { 0.02f, 0.03f, 0.05f, PANEL_ALPHA };
	// ADR-0104's formula, and it is the same one src/surface.c uses and out
	// of the same two constants — one surface, one scale, and the mouse
	// below divides by this very number.
	float pixels_per_millimetre = (float)target.height /
				      VOE_DEV_SURFACE_HIGH * VOE_DEV_UI_SCALE;
	voe_math_float2 millimetres;
	struct voe_base_arena_mark mark;
	voe_ui_node count_button;
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
				       .down = down });

	// The root is the whole surface, so the interface is laid out in the
	// same millimetres everything else on it is.
	voe_ui_column_begin(
		ui, (voe_ui_container){
			    .size = { .along = { VOE_UI_SIZE_FIXED,
						 millimetres.y },
				      .across = { VOE_UI_SIZE_FIXED,
						  millimetres.x } },
			    .pad = INTERFACE_INSET });
	// The spacer card 041 removes. See the header.
	voe_ui_box(ui, (voe_math_float2){ 0.0f, INTERFACE_TOP },
		   (voe_ui_sizing){ 0 });

	voe_ui_panel_begin(ui, "hud", 0, plate,
			   (voe_ui_container){ .across = VOE_UI_ACROSS_START,
					       .gap = 3.0f,
					       .pad = 4.0f });
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
