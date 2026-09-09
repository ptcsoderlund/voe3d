// The forty rectangles, in the order they are painted. Four groups, and each of
// them is there to show one thing the element path claims:
//
//   1  ONE BACKING PANEL, dark and see-through, which everything else stands on.
//      It is what makes the see-through row above it readable, and it is the
//      first thing submitted because order is paint order.
//   2  ONE CLIPPED BAR, twice as wide as its clip rectangle, so that exactly
//      half of it is drawn. The clip rectangle has no other caller in the engine
//      and this is where a person can see it working.
//   3  SIX BARS OF ONE COLOUR AT SIX ALPHAS, from solid to almost nothing. What
//      to look for is a smooth ramp: if the shader's premultiply were missing
//      the whole row would be too bright and the last bar would still be
//      obvious, and if it happened twice the row would fade away too early.
//   4  THIRTY-TWO SQUARES OF THIRTY-TWO COLOURS, eight across and four down.
//      That is the group that matters: thirty-two different colours cannot be
//      thirty-two draws of one shading record, and the draw count main.c prints
//      is what says they were not.
//
// THE COUNT IS ASSERTED AGAINST VOE_DEV_ELEMENTS AND NOT TRUSTED. Adding a
// rectangle here without changing that number is a refusal on the last submit,
// which would be a rectangle quietly missing from the picture; the assert at the
// bottom turns it into a stop instead.
//
// NOTHING IN HERE IS ANIMATED. The exhibit is the same every frame, which is
// what makes it a thing to look at rather than a thing to watch — and it is
// rebuilt and resubmitted every frame anyway, because the element buffer belongs
// to a frame slot and holds nothing between frames.
#include "elements.h"

#include <base/assert.h>

#include <math/float4.h>
#include <math/float4x4.h>

// The layout, in millimetres on the panel elements.h describes. Every number
// here is measured from the panel's top-left corner, because element space puts
// its origin there and runs y downwards.
#define PANEL_X 100.0f
#define PANEL_Y 55.0f
#define PANEL_WIDE 134.0f
#define PANEL_HIGH 74.0f

// Inside the panel, six millimetres in from its left edge and from its top.
#define INSET 6.0f

// The clipped bar: forty wide, and clipped to the left twenty of that.
#define BAR_Y 61.0f
#define BAR_WIDE 40.0f
#define BAR_HIGH 10.0f

// The see-through row: six bars, eighteen wide with two between them.
#define FADE_Y 75.0f
#define FADE_COUNT 6
#define FADE_WIDE 18.0f
#define FADE_HIGH 8.0f
#define FADE_GAP 2.0f

// The colour grid: eight across, four down, fourteen by eight with two between.
#define GRID_Y 89.0f
#define GRID_COLUMNS 8
#define GRID_ROWS 4
#define GRID_WIDE 14.0f
#define GRID_HIGH 8.0f
#define GRID_GAP 2.0f

// One rectangle, clipped to itself, which is what an element that is not meant
// to be clipped says: a zeroed clip rectangle clips everything away. See
// voe_render_element.
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

// A hue round the circle, as a linear colour. Six straight-line segments, which
// is the ordinary hue-to-rgb with the saturation and the value both at one — it
// is here so that thirty-two squares are thirty-two visibly different colours
// and not a gradient somebody has to be told is thirty-two things.
static voe_math_float4 hue(float turn, float alpha)
{
	float sixth = turn * 6.0f;
	int segment = (int)sixth % 6;
	float rise = sixth - (float)(int)sixth;
	float fall = 1.0f - rise;

	switch (segment) {
	case 0:
		return (voe_math_float4){ 1.0f, rise, 0.0f, alpha };
	case 1:
		return (voe_math_float4){ fall, 1.0f, 0.0f, alpha };
	case 2:
		return (voe_math_float4){ 0.0f, 1.0f, rise, alpha };
	case 3:
		return (voe_math_float4){ 0.0f, fall, 1.0f, alpha };
	case 4:
		return (voe_math_float4){ rise, 0.0f, 1.0f, alpha };
	default:
		return (voe_math_float4){ 1.0f, 0.0f, fall, alpha };
	}
}

bool voe_dev_elements_submit(voe_render_device *gpu)
{
	// Dark and three-quarters opaque, so the scene shows faintly through it
	// and the row above reads against it.
	voe_math_float4 backing = { 0.02f, 0.02f, 0.03f, 0.75f };
	// A warm colour for the clipped bar, so that the half that is drawn and
	// the half that is not are obvious against the backing.
	voe_math_float4 bar_colour = { 1.0f, 0.55f, 0.05f, 1.0f };
	voe_render_element bar;
	uint32_t submitted = 0;

	VOE_BASE_ASSERT(gpu != NULL, "submitting the element exhibit to no device");

	// 1. The backing panel. First, because everything else is painted over
	//    it and order is paint order on this path.
	if (!voe_render_frame_submit_element(
		    gpu, at(PANEL_X, PANEL_Y, PANEL_WIDE, PANEL_HIGH, backing)))
		return false;
	submitted++;

	// 2. The clipped bar: forty millimetres of rectangle and twenty
	//    millimetres of clip, so the right half of it is discarded in the
	//    fragment stage and the backing shows through where it would have
	//    been. A clip test that never ran would draw the whole forty.
	bar = at(PANEL_X + INSET, BAR_Y, BAR_WIDE, BAR_HIGH, bar_colour);
	bar.clip = (voe_math_float4){ PANEL_X + INSET, BAR_Y, BAR_WIDE / 2.0f,
				      BAR_HIGH };
	if (!voe_render_frame_submit_element(gpu, bar))
		return false;
	submitted++;

	// 3. The same colour at six alphas, left to right, solid first.
	for (int i = 0; i < FADE_COUNT; i++) {
		float alpha = 1.0f - (float)i * 0.18f;
		float x = PANEL_X + INSET + (float)i * (FADE_WIDE + FADE_GAP);
		voe_math_float4 colour = { 0.35f, 0.75f, 1.0f, alpha };

		if (!voe_render_frame_submit_element(
			    gpu, at(x, FADE_Y, FADE_WIDE, FADE_HIGH, colour)))
			return false;
		submitted++;
	}

	// 4. Thirty-two colours. The turn walks the whole circle across the
	//    grid read left to right and top to bottom, so no two squares are
	//    the same colour — which is the thing one draw of one shading record
	//    could not do.
	for (int row = 0; row < GRID_ROWS; row++) {
		for (int column = 0; column < GRID_COLUMNS; column++) {
			int n = row * GRID_COLUMNS + column;
			float turn = (float)n / (float)(GRID_ROWS * GRID_COLUMNS);
			float x = PANEL_X + INSET +
				  (float)column * (GRID_WIDE + GRID_GAP);
			float y = GRID_Y + (float)row * (GRID_HIGH + GRID_GAP);

			if (!voe_render_frame_submit_element(
				    gpu, at(x, y, GRID_WIDE, GRID_HIGH,
					    hue(turn, 1.0f))))
				return false;
			submitted++;
		}
	}

	// The number in elements.h is what the device was asked for, so a
	// rectangle added here without changing it would be refused above and
	// quietly missing from the picture. This is what makes that a stop.
	VOE_BASE_ASSERT(submitted == VOE_DEV_ELEMENTS,
			"the element exhibit submits a different number of rectangles from the one VOE_DEV_ELEMENTS names");

	return voe_render_frame_draw_elements(gpu,
					      voe_dev_elements_transform());
}

voe_math_float4x4 voe_dev_elements_transform(void)
{
	voe_math_float2 panel = { VOE_DEV_ELEMENTS_PANEL_WIDE,
				  VOE_DEV_ELEMENTS_PANEL_HIGH };

	return voe_render_element_transform(panel);
}
