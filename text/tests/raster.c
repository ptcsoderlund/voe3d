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
#include "../src/raster.h"
#include "../src/truetype.h"

#include <base/arena.h>
#include <math/float2.h>
#include <testing/test.h>

#include <string.h>

#define ARENA (64 * 1024)
#define SIZE 10

// One pixel per font unit, so a coordinate below is a pixel and the picture can
// be reasoned about by looking at the numbers.
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

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);

	check_a_square(arena);
	check_opposite_windings_leave_a_hole(arena);
	check_same_windings_stay_solid(arena);
	check_neither_direction_is_special(arena);
	check_a_partly_covered_pixel(arena);
	check_a_straight_curve(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
