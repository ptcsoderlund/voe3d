// The forty rectangles and the forty letters of the exhibit, in the order they
// are painted, and the badge's five afterwards. Six groups in the exhibit, and
// each of them is there to show one thing the element path claims:
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
//   5  ONE LINE OF WRITING, LARGE. What to look for is the edge of a letter:
//      it is a hard cut through a distance field, so a stem has straight sides
//      and a corner is a corner, at whatever size the window happens to be.
//      Nothing here is antialiased and it is not meant to look as though it is.
//   6  ONE LINE OF WRITING, SMALL — small enough to show what the sheet cannot
//      hold. The atlas samples the shape at a fixed resolution, so a stem
//      thinner than a texel or two is one it has no room to describe: such text
//      THINS and eventually breaks up rather than going blurry, which is the
//      thing text/include/text/font.h's "no analytic curves" paragraph is about
//      and is worth being able to see rather than only read.
//
// A LETTER IS A RECTANGLE THAT READS A SHEET, AND THAT IS ALL IT IS. Groups five
// and six go into the same buffer as groups one to four, in the same submission
// order, and out through the same draw command. Nothing here is a text system:
// this file asks the font where each character sits, multiplies by its own
// millimetres-per-em and turns the direction round once, and submits a record.
// Laying a string out properly — wrapping, alignment, a caret — is `ui`'s and it
// is not started here.
//
// THE COUNT IS ASSERTED AGAINST VOE_DEV_ELEMENTS AND NOT TRUSTED. Adding a
// rectangle here, or a letter to one of the strings, without changing the
// numbers in elements.h is a refusal on the last submit — which would be a
// rectangle or a letter quietly missing from the picture. The asserts at the
// bottom turn that into a stop instead, and there are two of them so that the
// message says which half drifted.
//
// NOTHING IN HERE IS ANIMATED. The exhibit is the same every frame, which is
// what makes it a thing to look at rather than a thing to watch — and it is
// rebuilt and resubmitted every frame anyway, because the element buffer belongs
// to a frame slot and holds nothing between frames.
//
// AND NOTHING IN HERE DRAWS. Each function submits records and stops; the range
// it filled is what main.c reads off voe_render_frame_elements_submitted and
// writes onto the panel component, and the draw system issues one draw per panel
// in layer and sort order. What that buys is the picture the checkpoint is for:
// a cube in front of the exhibit hides it, which a surface drawn after the world
// could not do.
#include "elements.h"

#include <base/assert.h>

#include <math/float4.h>

#include <text/font.h>

// The layout, in millimetres on the panel elements.h describes. Every number
// here is measured from the panel's top-left corner, because element space puts
// its origin there and runs y downwards.
//
// THE PANEL STARTS HIGHER THAN THE RECTANGLES DO, and the space above them is
// where the writing goes. Every rectangle below is at the millimetre it has
// always been at; only the backing grew upwards to stand the two lines on.
#define PANEL_X 100.0f
#define PANEL_Y 30.0f
#define PANEL_WIDE 134.0f
#define PANEL_HIGH 99.0f

// Inside the panel, six millimetres in from its left edge. Everything written
// below starts here.
#define INSET 6.0f

// The two lines of writing, by their BASELINES — which is what a font measures
// from and so what a caller placing characters has to think in. A letter's box
// reaches above it by however much the font says, and a descender reaches below.
//
// THE EM SIZES ARE THE POINT OF THE PAIR AND NOT A TASTE. Nine millimetres is
// large enough that a stem is many pixels across and the hard edge of the field
// is plainly a hard edge; two and a half is small enough that a stem is about
// one, which is where the sheet runs out of room to describe the shape and the
// letters thin. Both are drawn from the one atlas, at the one resolution, by the
// one shader — the difference is entirely what a distance field can and cannot
// do, which is what makes the two lines worth having beside each other.
#define TITLE_BASELINE 43.0f
#define TITLE_EM 9.0f
#define LABEL_BASELINE 52.0f
#define LABEL_EM 2.5f

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

// The badge's layout, in millimetres on the small panel elements.h describes.
// Everything is measured from its top-left corner, the same as the exhibit's.
#define BADGE_INSET 5.0f
#define BADGE_MARK 14.0f
#define BADGE_TICKS 3
#define BADGE_TICK_WIDE 26.0f
#define BADGE_TICK_HIGH 4.0f

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

// One line of writing, as one element per character, left to right from `x` and
// standing on `baseline`. Returns how many elements it submitted, or leaves
// `*ok` false if `render` refused one.
//
// THIS IS THE CONVERSION THE WHOLE TEXT HALF EXISTS FOR, AND IT IS FOUR LINES.
// `text` says where a character sits in EMS with +Y UP, because that is what a
// font knows and it does not know what a screen is. An element surface is
// MILLIMETRES with Y DOWN from its top-left corner. So the caller multiplies by
// its own millimetres-per-em — which is what makes a size measured from the font
// in hand rather than a number somebody typed — and subtracts from the baseline
// instead of adding to it. That subtraction is the only place the direction
// turns round, and it is not a second Y flip: the engine's one flip is in the
// viewport and voe_render_element_surface_matrix owns the element path's sign.
//
// THE BOX IS TAKEN WHOLE AND NOT TRIMMED. It is wider than the letter looks on
// every side, because the sheet carries the field on past the outline and the
// rectangle has to be big enough for the shader to see where the field says the
// edge is. Shrinking it to the letter gives clipped stems, which reads as a bad
// font rather than as a wrong rectangle.
//
// A SPACE IS NOT SUBMITTED. It advances the pen and draws nothing, so a record
// for it would spend a letter's worth of the frame's capacity on an empty
// rectangle.
//
// THE STRING IS ASCII AND A BYTE IS A CHARACTER HERE. Decoding UTF-8 is a thing
// `text` does inside its own layout and does not offer; a caller that needs it
// is `ui`, and this exhibit is not it.
static uint32_t write_line(voe_render_device *gpu, const voe_text_font *font,
			   const char *ascii, float x, float baseline, float em,
			   voe_math_float4 colour, bool *ok)
{
	uint32_t submitted = 0;
	float pen = x;

	for (const char *c = ascii; *c != '\0'; c++) {
		voe_text_glyph g = voe_text_font_glyph(font,
						       (unsigned char)*c);
		voe_render_element element;

		if (g.drawn) {
			element = (voe_render_element){
				.bounds = { pen + g.low.x * em,
					    baseline - g.high.y * em,
					    (g.high.x - g.low.x) * em,
					    (g.high.y - g.low.y) * em },
				// The panel, so a line running off the end of
				// it is cut rather than drawn over the scene.
				// That is what a panel does and it is the same
				// field the clipped bar above uses.
				.clip = { PANEL_X, PANEL_Y, PANEL_WIDE,
					  PANEL_HIGH },
				.colour = colour,
				.kind = VOE_RENDER_ELEMENT_GLYPH,
				.sheet_texture =
					voe_text_font_atlas(font).index,
				// Straight across: voe_text_glyph already
				// hands the sheet rectangle over in the shape
				// the record wants, corner and size, with its
				// corner paired to the box's top-left.
				.sheet = g.sheet,
			};

			if (!voe_render_frame_submit_element(gpu, element)) {
				*ok = false;
				return submitted;
			}
			submitted++;
		}
		pen += g.advance * em;
	}
	return submitted;
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

bool voe_dev_elements_submit(voe_render_device *gpu, const voe_text_font *font)
{
	// Dark and three-quarters opaque, so the scene shows faintly through it
	// and the row above reads against it.
	voe_math_float4 backing = { 0.02f, 0.02f, 0.03f, 0.75f };
	// A warm colour for the clipped bar, so that the half that is drawn and
	// the half that is not are obvious against the backing.
	voe_math_float4 bar_colour = { 1.0f, 0.55f, 0.05f, 1.0f };
	// Nearly white, so the large line is read against the backing rather
	// than against a hue.
	voe_math_float4 title_colour = { 0.90f, 0.92f, 0.95f, 1.0f };
	// Dimmer, because the small line is a caption and because a lower
	// contrast is where a thinning stem is easiest to see.
	voe_math_float4 label_colour = { 0.55f, 0.62f, 0.70f, 1.0f };
	voe_render_element bar;
	uint32_t submitted = 0;
	uint32_t glyphs = 0;
	bool wrote = true;

	VOE_BASE_ASSERT(gpu != NULL, "submitting the element exhibit to no device");
	VOE_BASE_ASSERT(font != NULL,
			"the element exhibit writes, so it needs a font");

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

	VOE_BASE_ASSERT(submitted == VOE_DEV_ELEMENTS_RECTANGLES,
			"the element exhibit submits a different number of rectangles from the one VOE_DEV_ELEMENTS_RECTANGLES names");

	// 5 and 6. The writing, over the rectangles because it is submitted
	//    after them. Two lines, one large and one small, and the small one
	//    is there to be looked at closely — see the header.
	glyphs += write_line(gpu, font, "Glyphs are elements",
			     PANEL_X + INSET, TITLE_BASELINE, TITLE_EM,
			     title_colour, &wrote);
	glyphs += write_line(gpu, font, "one draw for the whole panel",
			     PANEL_X + INSET, LABEL_BASELINE, LABEL_EM,
			     label_colour, &wrote);
	if (!wrote)
		return false;
	submitted += glyphs;

	// The numbers in elements.h are what the device was asked for, so a
	// rectangle added here — or a letter, which is what changing a string
	// does — without changing them would be refused above and quietly
	// missing from the picture. This is what makes that a stop.
	VOE_BASE_ASSERT(glyphs == VOE_DEV_ELEMENTS_GLYPHS,
			"the element exhibit writes a different number of letters from the one VOE_DEV_ELEMENTS_GLYPHS names");
	VOE_BASE_ASSERT(submitted == VOE_DEV_ELEMENTS,
			"the element exhibit submits a different number of elements from the one VOE_DEV_ELEMENTS names");

	return true;
}

// The badge: a dark plate with a bright corner mark and three ticks down its
// left edge.
//
// IT IS ASYMMETRIC ON PURPOSE, exactly as render/tests/elements.c's arrangement
// is. The badge is in the overlay, where nothing else is behind it to say which
// way up it is, so a surface matrix with its Y sign the wrong way round would
// draw a badge that looked perfectly reasonable. The mark is in ONE corner and
// the ticks are on ONE edge, so upside down is visible.
bool voe_dev_elements_badge_submit(voe_render_device *gpu)
{
	// A deep violet, mostly opaque: a colour nothing else in the scene is,
	// so that "the overlay panel is the one nothing covers" is a thing a
	// person can pick out rather than a dark shape among dark shapes.
	voe_math_float4 plate = { 0.10f, 0.04f, 0.32f, 0.90f };
	// Warm, and only in the top-left corner.
	voe_math_float4 mark = { 1.0f, 0.45f, 0.10f, 1.0f };
	voe_math_float4 tick = { 0.35f, 0.75f, 1.0f, 1.0f };
	uint32_t submitted = 0;

	VOE_BASE_ASSERT(gpu != NULL, "submitting the badge to no device");

	if (!voe_render_frame_submit_element(
		    gpu, at(0.0f, 0.0f, VOE_DEV_BADGE_WIDE, VOE_DEV_BADGE_HIGH,
			    plate)))
		return false;
	submitted++;

	// The corner mark, at the badge's own origin, which is its TOP-LEFT.
	if (!voe_render_frame_submit_element(
		    gpu, at(BADGE_INSET, BADGE_INSET, BADGE_MARK, BADGE_MARK,
			    mark)))
		return false;
	submitted++;

	// Three ticks down the left edge, below the mark, each one further down
	// than the last — so which end of the badge is the top is a thing the
	// picture says rather than a thing to take on trust.
	for (int i = 0; i < BADGE_TICKS; i++) {
		float y = BADGE_INSET + BADGE_MARK + BADGE_INSET +
			  (float)i * (BADGE_TICK_HIGH + BADGE_INSET);

		if (!voe_render_frame_submit_element(
			    gpu, at(BADGE_INSET, y, BADGE_TICK_WIDE,
				    BADGE_TICK_HIGH, tick)))
			return false;
		submitted++;
	}

	VOE_BASE_ASSERT(submitted == VOE_DEV_BADGE_ELEMENTS,
			"the badge submits a different number of elements from the one VOE_DEV_BADGE_ELEMENTS names");
	return true;
}
