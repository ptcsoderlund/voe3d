// The fill rule, and it is the claim in this folder most worth a test. Needs no
// graphics card.
//
// NON-ZERO AND EVEN-ODD AGREE ABOUT ALMOST EVERYTHING, WHICH IS WHY THIS IS
// SUBTLE. A letter with one counter — an `o`, an `e` — comes out right under
// either rule, because its two contours run opposite ways round. The two rules
// only disagree when contours run the SAME way round, and then non-zero fills
// the overlap while even-odd punches it out. So a test that only checked an `o`
// would pass with the wrong rule implemented, and a font whose glyphs happened
// to overlap would then come out full of holes.
//
// The two cases below are exactly that pair: the same two squares, differing
// only in which way round the inner one is wound. Opposite is a hole and same is
// solid, and no rule but non-zero winding gives both answers.
//
// THE OUTLINES ARE BUILT HERE AND NOT READ OUT OF THE FONT. A rasteriser test
// wants a shape whose answer is known by looking at it, and a real glyph's is
// not — every claim would then be a number copied out of this code's own output.
//
// ---- AND THE FIELD, WHOSE CLAIM IS A CORNER AND NOT A LETTER ----
//
// THE COLOURING FAILS QUIETLY, WHICH IS WHY IT IS TESTED ON A RIGHT ANGLE. A
// mis-assigned corner is a small notch in one letter; it survives every test
// that is a whole word, and it survives a test that only asks whether the field
// is positive inside and negative outside, because it is. So the claim below is
// made about one corner of one square, where the right answer can be worked out
// on paper.
//
// EVERY CLAIM IS ABOUT THE MEDIAN, BECAUSE THE MEDIAN IS WHAT THE SHADER READS.
// The three channels are allowed to disagree — that is the mechanism, not a
// fault — and a channel on its own means nothing. What has to be true is what
// render/shaders/draw.slang computes from them, so that is what is measured, and
// the one claim made about the three separately is that at a corner they are not
// all the same. If they were, the sheet would be an ordinary distance field
// wearing three channels and every corner in the font would be an arc.
#include "../src/raster.h"
#include "../src/truetype.h"

#include <base/arena.h>
#include <math/float2.h>
#include <testing/test.h>

#include <string.h>

#define ARENA (64 * 1024)
#define SIZE 12

// One pixel per font unit, so a coordinate below is a pixel and the picture can
// be reasoned about by looking at the numbers. SIZE is twelve rather than the
// ten the fill alone needed, because the field's claims are about texels several
// away from an edge and a shape has to have room round it for that.
#define SCALE 1.0f

// Everything is drawn into a SIZE by SIZE bitmap with font-unit (0, 0) at its
// bottom-left corner. The y is SIZE and not 0 because the bitmap counts
// downwards while the font counts upwards — see raster.h.
static const voe_math_float2 ORIGIN = { 0.0f, (float)SIZE };

// A square from (low, low) to (high, high), wound anticlockwise in font space
// when `forward` and clockwise when not.
static void square(voe_text_truetype_point *into, float low, float high,
		   bool forward)
{
	const float x[4] = { low, high, high, low };
	const float y[4] = { low, low, high, high };

	for (int i = 0; i < 4; i++) {
		int at = forward ? i : 3 - i;

		into[i].point = (voe_math_float2){ x[at], y[at] };
		into[i].on_curve = true;
	}
}

static void fill(voe_text_truetype_outline *outline, uint8_t *coverage,
		 voe_base_arena *arena)
{
	memset(coverage, 0, SIZE * SIZE);
	voe_text_raster_fill(outline, SCALE, ORIGIN, coverage, SIZE, SIZE,
			     arena);
}

// The pixel whose centre is at (x + 0.5, y + 0.5) in font space, which is the
// row SIZE - 1 - y of the bitmap.
static uint8_t at(const uint8_t *coverage, int x, int y)
{
	return coverage[(SIZE - 1 - y) * SIZE + x];
}

// One filled square, which is the claim everything else rests on: inside is
// solid, outside is nothing, and neither is off by a row.
static void check_a_square(voe_base_arena *arena)
{
	voe_text_truetype_point points[4];
	uint16_t ends[1] = { 3 };
	voe_text_truetype_outline outline = { points, ends, 4, 1 };
	uint8_t coverage[SIZE * SIZE];

	square(points, 2.0f, 8.0f, true);
	fill(&outline, coverage, arena);

	VOE_TEST_CHECK_INT(at(coverage, 5, 5), 255);
	VOE_TEST_CHECK_INT(at(coverage, 2, 2), 255);
	VOE_TEST_CHECK_INT(at(coverage, 7, 7), 255);
	VOE_TEST_CHECK_INT(at(coverage, 1, 5), 0);
	VOE_TEST_CHECK_INT(at(coverage, 8, 5), 0);
	// Below and above the square. These two are the ones that fail when the
	// Y flip is missing or applied twice, and they fail the same way round
	// either time.
	VOE_TEST_CHECK_INT(at(coverage, 5, 1), 0);
	VOE_TEST_CHECK_INT(at(coverage, 5, 8), 0);
}

// A square inside a square, wound the other way: the middle is a hole. This is
// the counter in an `o`, an `e`, an `a` and a `B`, and it is what fills in solid
// when a rasteriser sums crossings without their sign.
static void check_opposite_windings_leave_a_hole(voe_base_arena *arena)
{
	voe_text_truetype_point points[8];
	uint16_t ends[2] = { 3, 7 };
	voe_text_truetype_outline outline = { points, ends, 8, 2 };
	uint8_t coverage[SIZE * SIZE];

	square(points, 1.0f, 9.0f, true);
	square(points + 4, 4.0f, 6.0f, false);
	fill(&outline, coverage, arena);

	VOE_TEST_CHECK_INT(at(coverage, 2, 5), 255);
	VOE_TEST_CHECK_INT(at(coverage, 5, 5), 0);
}

// The same two squares wound the same way: no hole. Even-odd would punch one
// here, which is what makes this the half of the pair that pins the rule down.
static void check_same_windings_stay_solid(voe_base_arena *arena)
{
	voe_text_truetype_point points[8];
	uint16_t ends[2] = { 3, 7 };
	voe_text_truetype_outline outline = { points, ends, 8, 2 };
	uint8_t coverage[SIZE * SIZE];

	square(points, 1.0f, 9.0f, true);
	square(points + 4, 4.0f, 6.0f, true);
	fill(&outline, coverage, arena);

	VOE_TEST_CHECK_INT(at(coverage, 2, 5), 255);
	VOE_TEST_CHECK_INT(at(coverage, 5, 5), 255);
}

// And the same hole again with the outer square wound the other way round, so
// that neither direction is the one this happens to work for. TrueType says
// outer contours are clockwise in font space and plenty of fonts disagree with
// it; what matters is that the two are opposite, not which is which.
static void check_neither_direction_is_special(voe_base_arena *arena)
{
	voe_text_truetype_point points[8];
	uint16_t ends[2] = { 3, 7 };
	voe_text_truetype_outline outline = { points, ends, 8, 2 };
	uint8_t coverage[SIZE * SIZE];

	square(points, 1.0f, 9.0f, false);
	square(points + 4, 4.0f, 6.0f, true);
	fill(&outline, coverage, arena);

	VOE_TEST_CHECK_INT(at(coverage, 2, 5), 255);
	VOE_TEST_CHECK_INT(at(coverage, 5, 5), 0);
}

// A half-covered pixel comes back about half covered. The anti-aliasing is the
// reason this rasteriser exists rather than a fill that writes ones and noughts,
// and an edge that snapped to a pixel boundary would be invisible in a test that
// only ever measured whole pixels.
static void check_a_partly_covered_pixel(voe_base_arena *arena)
{
	voe_text_truetype_point points[4];
	uint16_t ends[1] = { 3 };
	voe_text_truetype_outline outline = { points, ends, 4, 1 };
	uint8_t coverage[SIZE * SIZE];

	// A tall rectangle whose right edge falls down the middle of the column
	// at x = 5. Tall rather than square, so that the row being measured is
	// covered from top to bottom and the only fraction in the answer is the
	// one in x.
	points[0] = (voe_text_truetype_point){ { 2.0f, 1.0f }, true };
	points[1] = (voe_text_truetype_point){ { 5.5f, 1.0f }, true };
	points[2] = (voe_text_truetype_point){ { 5.5f, 9.0f }, true };
	points[3] = (voe_text_truetype_point){ { 2.0f, 9.0f }, true };
	fill(&outline, coverage, arena);

	VOE_TEST_CHECK_INT(at(coverage, 4, 5), 255);
	VOE_TEST_CHECK(at(coverage, 5, 5) > 100 && at(coverage, 5, 5) < 155);
	VOE_TEST_CHECK_INT(at(coverage, 6, 5), 0);
}

// A curve is flattened rather than sampled, and a quadratic that is actually
// straight has to come out straight. Three points in a line with the middle one
// off the curve describe a straight edge; if the flattening were wrong the shape
// would bulge and the pixel just outside it would pick something up.
static void check_a_straight_curve(voe_base_arena *arena)
{
	voe_text_truetype_point points[4];
	uint16_t ends[1] = { 3 };
	voe_text_truetype_outline outline = { points, ends, 4, 1 };
	uint8_t coverage[SIZE * SIZE];

	points[0] = (voe_text_truetype_point){ { 2.0f, 2.0f }, true };
	points[1] = (voe_text_truetype_point){ { 8.0f, 2.0f }, true };
	// On the line from (8, 2) to (8, 8), so the curve through it is that
	// line.
	points[2] = (voe_text_truetype_point){ { 8.0f, 5.0f }, false };
	points[3] = (voe_text_truetype_point){ { 8.0f, 8.0f }, true };
	fill(&outline, coverage, arena);

	VOE_TEST_CHECK_INT(at(coverage, 7, 3), 255);
	VOE_TEST_CHECK_INT(at(coverage, 8, 5), 0);
}

// The field of the same square the fill tests use, so that one shape is reasoned
// about once. Bitmap coordinates throughout: the helpers below index rows down
// the image rather than up from the baseline, because a distance is about where
// a texel is and not about where the baseline was.
static void field(voe_text_truetype_outline *outline, uint8_t *into,
		  voe_base_arena *arena)
{
	voe_text_raster_field(outline, SCALE, ORIGIN, into, SIZE, SIZE, arena);
}

// One channel of one texel, back as the signed distance in texels it stands for.
// The inverse of what raster.c encodes, so a claim below can be written as a
// distance rather than as a byte.
static float channel_at(const uint8_t *into, int x, int y, int channel)
{
	uint8_t byte = into[((y * SIZE) + x) * VOE_TEXT_FIELD_CHANNELS +
			    channel];

	return ((float)byte / 255.0f * 2.0f - 1.0f) * VOE_TEXT_FIELD_SPREAD;
}

// What the shader reads: the middle of the three.
static float median_at(const uint8_t *into, int x, int y)
{
	float a = channel_at(into, x, y, 0);
	float b = channel_at(into, x, y, 1);
	float c = channel_at(into, x, y, 2);
	float low = a < b ? a : b;
	float high = a < b ? b : a;

	return high < c ? high : (low > c ? low : c);
}

// A square whose bitmap corners are (3, 3) and (9, 9), wound the way TrueType
// winds an outer contour. Every claim below is about this one shape.
static void field_square(voe_text_truetype_point *into)
{
	square(into, 3.0f, 9.0f, false);
}

// Positive inside, negative outside, and the halfway mark on the outline. This
// is the claim everything else rests on, and it is the one that fails if the
// sign convention is the wrong way round — which would draw every glyph as its
// own background.
static void check_the_field_has_a_side(voe_base_arena *arena)
{
	voe_text_truetype_point points[4];
	uint16_t ends[1] = { 3 };
	voe_text_truetype_outline outline = { points, ends, 4, 1 };
	uint8_t into[SIZE * SIZE * VOE_TEXT_FIELD_CHANNELS];

	field_square(points);
	field(&outline, into, arena);

	// Texel (6, 6) is centred at (6.5, 6.5) and the nearest edges are the
	// two at 9, so it is two and a half texels inside.
	VOE_TEST_CHECK(median_at(into, 6, 6) > 2.4f);
	VOE_TEST_CHECK(median_at(into, 6, 6) < 2.6f);
	// And (0, 0) is centred at (0.5, 0.5), two and a half texels outside
	// both of the edges at 3.
	VOE_TEST_CHECK(median_at(into, 0, 0) < -2.4f);
	VOE_TEST_CHECK(median_at(into, 0, 0) > -2.6f);
}

// A texel beside the middle of an edge is that edge's perpendicular distance
// away, and nothing else. No corner is near enough to matter, so this is the
// case the three channels have nothing to disagree about — and it is what pins
// the scale of the encoding down, because a spread read as half or twice what it
// is would still be positive inside and negative outside.
static void check_a_straight_edge_is_its_own_distance(voe_base_arena *arena)
{
	voe_text_truetype_point points[4];
	uint16_t ends[1] = { 3 };
	voe_text_truetype_outline outline = { points, ends, 4, 1 };
	uint8_t into[SIZE * SIZE * VOE_TEXT_FIELD_CHANNELS];

	field_square(points);
	field(&outline, into, arena);

	// Texel (6, 1) has its centre at (6.5, 1.5) and the top edge is at
	// y = 3, so it is a texel and a half outside.
	VOE_TEST_CHECK(median_at(into, 6, 1) < -1.3f);
	VOE_TEST_CHECK(median_at(into, 6, 1) > -1.7f);
	// And (6, 4) is centred at (6.5, 4.5), a texel and a half inside.
	VOE_TEST_CHECK(median_at(into, 6, 4) > 1.3f);
	VOE_TEST_CHECK(median_at(into, 6, 4) < 1.7f);
}

// THE CLAIM THIS WHOLE FILE'S SECOND HALF EXISTS FOR. Diagonally outside a right
// angle, an ordinary distance field says how far the CORNER POINT is — texel
// (1, 1) is centred at (1.5, 1.5) and the corner is at (3, 3), so that is 2.12
// texels — and reading it back through a linear filter turns the right angle
// into an arc of that radius. The median says how far the nearer EDGE's line is
// instead, which is 1.5, and where two of those lines cross is exactly the
// corner.
//
// So the test is that the median is nearer than the corner point is. It is not a
// tolerance on a number this code produced: 1.5 and 2.12 are the two answers the
// two designs give, and no rounding gets from one to the other.
static void check_a_corner_is_not_rounded_off(voe_base_arena *arena)
{
	voe_text_truetype_point points[4];
	uint16_t ends[1] = { 3 };
	voe_text_truetype_outline outline = { points, ends, 4, 1 };
	uint8_t into[SIZE * SIZE * VOE_TEXT_FIELD_CHANNELS];
	float a;
	float b;
	float c;

	field_square(points);
	field(&outline, into, arena);

	VOE_TEST_CHECK(median_at(into, 1, 1) > -1.7f);
	VOE_TEST_CHECK(median_at(into, 1, 1) < -1.3f);

	// And the three do not agree, which is the mechanism itself: a sheet
	// whose channels were equal everywhere would be one distance field in
	// three copies and the check above could only pass by accident.
	//
	// MEASURED OFF THE DIAGONAL, BECAUSE ON IT THEY LEGITIMATELY AGREE.
	// Texel (1, 1) is the same distance from both edges' lines, so all
	// three channels answer the same number and there is nothing to see.
	// Texel (1, 2) is centred at (1.5, 2.5) — half a texel above the top
	// edge's line and a texel and a half left of the left edge's — so the
	// channels carrying one edge and the channels carrying the other have
	// to differ, and the median of them is the further of the two.
	a = channel_at(into, 1, 2, 0);
	b = channel_at(into, 1, 2, 1);
	c = channel_at(into, 1, 2, 2);
	VOE_TEST_CHECK(a != b || b != c);
	VOE_TEST_CHECK(median_at(into, 1, 2) > -1.7f);
	VOE_TEST_CHECK(median_at(into, 1, 2) < -1.3f);
}

// A counter is a hole in the field as well as in the fill. Two squares wound
// opposite ways: the middle is outside the shape, so the field there is
// negative. The sign of a field comes from which way round a contour runs, and
// this is the case where getting that backwards is visible — a letter with its
// counters filled in.
static void check_a_counter_is_outside(voe_base_arena *arena)
{
	voe_text_truetype_point points[8];
	uint16_t ends[2] = { 3, 7 };
	voe_text_truetype_outline outline = { points, ends, 8, 2 };
	uint8_t into[SIZE * SIZE * VOE_TEXT_FIELD_CHANNELS];

	// Bitmap (1, 1) to (11, 11) with a hole from (5, 5) to (7, 7).
	square(points, 1.0f, 11.0f, false);
	square(points + 4, 5.0f, 7.0f, true);
	field(&outline, into, arena);

	VOE_TEST_CHECK(median_at(into, 2, 6) > 0.0f);
	VOE_TEST_CHECK(median_at(into, 6, 6) < 0.0f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);

	check_a_square(arena);
	check_opposite_windings_leave_a_hole(arena);
	check_same_windings_stay_solid(arena);
	check_neither_direction_is_special(arena);
	check_a_partly_covered_pixel(arena);
	check_a_straight_curve(arena);
	check_the_field_has_a_side(arena);
	check_a_straight_edge_is_its_own_distance(arena);
	check_a_corner_is_not_rounded_off(arena);
	check_a_counter_is_outside(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
