// The screen-filling surface's content: a plate against the right edge, a marked
// square inside it, and a row of ticks running away to the right along the top
// edge.
//
// THE PLATE IS ON THE RIGHT BECAUSE THE READOUT OWNS THE TOP-LEFT. Card 032 put
// it in that corner and card 040 found what that meant: the readout's numbers
// were behind it and unreadable. The readout is a mesh in the world and has been
// in that corner since long before either card, so the plate is the newcomer and
// the plate is what moved.
//
// AND ITS x IS ANCHORED RATHER THAN AUTHORED, WHICH IS THE ONE THING HERE THAT
// IS NOT AT A FIXED MILLIMETRE. A plate authored at the right edge of a 240 mm
// surface would be at 188 mm and would fall off a narrow window entirely — and a
// plate nobody can see says nothing about whether anything changed size. So it
// is placed a fixed distance from wherever the surface's right edge turned out
// to be, exactly as the bar along the bottom is placed from its bottom edge. The
// TICKS are the fixed-position demonstration and they are untouched; this is the
// piece that has to stay on screen for that demonstration to be looked at.
//
// THE TICKS ARE THE POINT AND THEY ARE MEANT TO RUN OFF THE EDGE. Each one sits
// at a fixed millimetre position further right than the last, and the last of
// them is past where a narrow window's millimetres stop. Drag the window narrow
// and they disappear one at a time from the right, at exactly the same size and
// exactly the same spacing as before — which is what "the same content with less
// room" looks like, and what the old stretching behaviour could not do. Drag it
// wide and they all come back, still the same size.
//
// AND THE MARKED SQUARE IS THE SHAPE CLAIM. It is square in millimetres, so it
// has to be square on screen in every window shape there is. Two independent
// scales — the thing this surface used to have — turn it into an oblong the
// moment the window stops being 16:9, and a square is the one shape where that
// is impossible to miss.
//
// IT IS DELIBERATELY SMALL AND HAS NO WRITING. The exhibit is what shows off the
// element path; this shows off one decision, and a second exhibit here would
// make the thing being looked at harder to see. No font, no glyphs.
//
// EVERY COLOUR IN HERE IS LINEAR, as everything crossing render's boundary is.
#include "surface.h"

#include <base/assert.h>

#include <math/float2.h>
#include <math/float4.h>

// The plate: how big it is, how far in from the surface's right edge it sits,
// and how far down. The y is a plain authored millimetre and is well below the
// readout, which ends around forty; the x is worked out from the surface's own
// width — see the header.
#define PLATE_INSET 6.0f
#define PLATE_Y 60.0f
#define PLATE_WIDE 46.0f
#define PLATE_HIGH 30.0f

// The square inside it, from the plate's own top-left corner: square in
// millimetres, so square on screen. See the header.
#define SQUARE_INSET 6.0f
#define SQUARE_SIDE 18.0f

// The ticks along the top edge. The spacing is fixed, and the last one is at
// 6 + 8 * 27 = 222 mm — inside a 16:9 surface, which is 240 mm across, and well
// outside a narrow one.
#define TICK_X 6.0f
#define TICK_Y 40.0f
#define TICK_STEP 27.0f
#define TICK_WIDE 5.0f
#define TICK_HIGH 12.0f

static voe_render_element at(float x, float y, float w, float h,
			     voe_math_float4 colour)
{
	return (voe_render_element){
		.bounds = { x, y, w, h },
		.clip = { x, y, w, h },
		.colour = colour,
		.kind = VOE_RENDER_ELEMENT_SOLID,
	};
}

bool voe_dev_surface_draw(voe_render_device *gpu, voe_platform_size target)
{
	voe_math_float4 plate = { 0.02f, 0.04f, 0.06f, 0.80f };
	voe_math_float4 square = { 0.95f, 0.95f, 0.98f, 1.0f };
	voe_math_float4 tick = { 1.0f, 0.35f, 0.15f, 1.0f };
	// ADR-0104'S WHOLE FORMULA, AND THERE IS NOTHING ELSE TO IT. The window's
	// height divided by the millimetres the surface is meant to be tall,
	// times whatever calibration the person at the screen chose. No display
	// is read, on any platform, and `platform` has grown no surface for one.
	float pixels_per_millimetre = (float)target.height /
				      VOE_DEV_SURFACE_HIGH * VOE_DEV_UI_SCALE;
	// And how many millimetres that leaves, on both axes, from the one
	// number. This is what the surface is laid out in and what its transform
	// is given — see voe_render_element_surface_size for why it is handed
	// back rather than buried in the matrix.
	voe_math_float2 millimetres;
	// Where the plate ends up, once the surface's width is known.
	float plate_x;
	uint32_t first;
	uint32_t submitted = 0;

	VOE_BASE_ASSERT(gpu != NULL, "drawing the screen-filling surface on no device");

	// A window with no area never gets here — the loop skips the whole frame
	// when _begin says there is nothing to draw into — but the scale above
	// would be nought if it did, and that asserts inside render rather than
	// reaching a matrix.
	if (target.height == 0 || target.width == 0)
		return true;

	millimetres = voe_render_element_surface_size(target,
						      pixels_per_millimetre);

	// The range this surface fills, taken before anything is submitted. It
	// is not nought: the two panels have already submitted theirs into the
	// same buffer this frame.
	first = voe_render_frame_elements_submitted(gpu);

	// Against the right edge, and never off the left one: a window narrower
	// than the plate is wide would otherwise push it out of sight
	// altogether, and the card that moved it here asks for it to stay at
	// least partly visible at any width worth looking at.
	plate_x = millimetres.x - PLATE_WIDE - PLATE_INSET;
	if (plate_x < PLATE_INSET)
		plate_x = PLATE_INSET;

	if (!voe_render_frame_submit_element(
		    gpu, at(plate_x, PLATE_Y, PLATE_WIDE, PLATE_HIGH, plate)))
		return false;
	submitted++;

	if (!voe_render_frame_submit_element(
		    gpu, at(plate_x + SQUARE_INSET, PLATE_Y + SQUARE_INSET,
			    SQUARE_SIDE, SQUARE_SIDE, square)))
		return false;
	submitted++;

	for (int i = 0; i < VOE_DEV_SURFACE_TICKS; i++) {
		float x = TICK_X + (float)i * TICK_STEP;

		if (!voe_render_frame_submit_element(
			    gpu, at(x, TICK_Y, TICK_WIDE, TICK_HIGH, tick)))
			return false;
		submitted++;
	}

	// A bar along the very bottom of the surface, the full width of whatever
	// the window turned out to be. It is the one thing here that is measured
	// from the surface rather than authored at a fixed place, and it is what
	// says where the millimetres actually stopped: its right end is the
	// window's right edge, so the ticks that ran past it really did run off.
	if (!voe_render_frame_submit_element(
		    gpu, at(0.0f, millimetres.y - 3.0f, millimetres.x, 3.0f,
			    plate)))
		return false;
	submitted++;

	VOE_BASE_ASSERT(submitted == VOE_DEV_SURFACE_ELEMENTS,
			"the screen-filling surface submits a different number of elements from the one VOE_DEV_SURFACE_ELEMENTS names");

	return voe_render_frame_draw_elements(
		gpu, voe_render_element_transform(millimetres), first,
		submitted);
}
