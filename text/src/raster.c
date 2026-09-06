// The fill. Two passes over the outline — one to count the line segments the
// curves become and one to write them — then one pass per row of pixels. See
// raster.h for the fill rule, the anti-aliasing and where the Y flip is.
//
// TWO PASSES BECAUSE THE ARRAY IS PUSHED ONCE. How many segments a curve becomes
// depends on how curved it is in pixels, so the count is not known until the
// flattening has been worked out; and an arena's pushes are not guaranteed to be
// next to each other (base/arena.h), so growing an array by pushing again is not
// available. Counting first is the cheaper half of the same arithmetic.
#include "raster.h"

#include <base/assert.h>

#include <math.h>

// One flattened edge, in bitmap pixels. Horizontal edges are dropped as they are
// made: they cross no scanline and carry no winding, and keeping them would mean
// testing for a zero denominator in the innermost loop below.
struct edge {
	float x0;
	float y0;
	float x1;
	float y1;
};

// A scanline crossing: where an edge cuts this height, and which way it was
// going. The direction is the whole of the non-zero winding rule.
struct crossing {
	float x;
	int32_t direction;
};

// The state the two passes share. In counting mode `edges` is NULL and only
// `count` moves.
struct flattener {
	struct edge *edges;
	uint32_t count;
	uint32_t capacity;
};

static void emit(struct flattener *f, voe_math_float2 a, voe_math_float2 b)
{
	if (a.y == b.y)
		return;
	if (f->edges != NULL) {
		VOE_BASE_ASSERT(f->count < f->capacity,
				"the counting pass and the writing pass disagree");
		f->edges[f->count] = (struct edge){ a.x, a.y, b.x, b.y };
	}
	f->count++;
}

// How many straight pieces a quadratic has to be cut into for the chord of each
// piece to sit within the tolerance of the curve.
//
// THE ARITHMETIC, BECAUSE IT IS SHORT AND OTHERWISE LOOKS LIKE A GUESS. The
// midpoint of a quadratic is (p0 + 2c + p1)/4 and the midpoint of its chord is
// (p0 + p1)/2, so the curve leaves the chord by |p0 - 2c + p1|/4. Splitting it
// into n equal pieces divides that second difference by n², so n pieces leave the
// chord by |p0 - 2c + p1|/(4n²) — which is the number solved for n below.
static uint32_t quadratic_steps(voe_math_float2 p0, voe_math_float2 c,
				voe_math_float2 p1)
{
	float dx = p0.x - 2.0f * c.x + p1.x;
	float dy = p0.y - 2.0f * c.y + p1.y;
	float deviation = sqrtf(dx * dx + dy * dy) / 4.0f;
	float steps;

	if (deviation <= VOE_TEXT_RASTER_TOLERANCE)
		return 1;
	steps = ceilf(sqrtf(deviation / VOE_TEXT_RASTER_TOLERANCE));
	// A curve that has come out of a corrupt outline can ask for any number
	// at all; thirty-two pieces is far past the point where more of them
	// changes a pixel.
	if (steps > 32.0f)
		return 32;
	return (uint32_t)steps;
}

static void quadratic(struct flattener *f, voe_math_float2 p0,
		      voe_math_float2 c, voe_math_float2 p1)
{
	uint32_t steps = quadratic_steps(p0, c, p1);
	voe_math_float2 previous = p0;

	for (uint32_t i = 1; i <= steps; i++) {
		float t = (float)i / (float)steps;
		float u = 1.0f - t;
		voe_math_float2 at = {
			u * u * p0.x + 2.0f * u * t * c.x + t * t * p1.x,
			u * u * p0.y + 2.0f * u * t * c.y + t * t * p1.y,
		};

		emit(f, previous, at);
		previous = at;
	}
}

// Font units to bitmap pixels, and THE ONE PLACE THIS FOLDER NEGATES Y. Font
// space is Y-up from the baseline; a bitmap's rows run down the image.
static voe_math_float2 to_bitmap(voe_math_float2 point, float scale,
				 voe_math_float2 origin)
{
	return (voe_math_float2){ origin.x + point.x * scale,
				  origin.y - point.y * scale };
}

// One contour, walked into edges. TrueType stores a contour as a ring of points
// that are alternately anchors and control points, with two rules that are the
// whole of the fiddliness: two control points in a row imply an anchor exactly
// between them, and a contour is allowed to begin on a control point.
static void flatten_contour(struct flattener *f,
			    const voe_text_truetype_outline *outline,
			    uint16_t first, uint16_t last, float scale,
			    voe_math_float2 origin)
{
	uint16_t count = (uint16_t)(last - first + 1);
	int32_t on_curve = -1;
	voe_math_float2 start;
	voe_math_float2 pen;
	voe_math_float2 control = { 0.0f, 0.0f };
	bool have_control = false;
	uint16_t begin;
	uint16_t steps;

	if (count < 2)
		return;

	for (uint16_t i = 0; i < count; i++) {
		if (outline->points[first + i].on_curve) {
			on_curve = i;
			break;
		}
	}

	if (on_curve < 0) {
		// A contour of nothing but control points. The anchor it starts
		// from is the implied one between the last and the first, and
		// then every point in the ring is a control point.
		voe_math_float2 a = outline->points[first + count - 1].point;
		voe_math_float2 b = outline->points[first].point;

		start = to_bitmap((voe_math_float2){ (a.x + b.x) * 0.5f,
						     (a.y + b.y) * 0.5f },
				  scale, origin);
		begin = 0;
		steps = count;
	} else {
		start = to_bitmap(outline->points[first + on_curve].point,
				  scale, origin);
		begin = (uint16_t)(on_curve + 1);
		steps = (uint16_t)(count - 1);
	}

	pen = start;
	for (uint16_t k = 0; k < steps; k++) {
		uint16_t i = (uint16_t)((begin + k) % count);
		const voe_text_truetype_point *p = &outline->points[first + i];
		voe_math_float2 at = to_bitmap(p->point, scale, origin);

		if (p->on_curve) {
			if (have_control) {
				quadratic(f, pen, control, at);
				have_control = false;
			} else {
				emit(f, pen, at);
			}
			pen = at;
			continue;
		}

		if (have_control) {
			voe_math_float2 implied = { (control.x + at.x) * 0.5f,
						    (control.y + at.y) * 0.5f };

			quadratic(f, pen, control, implied);
			pen = implied;
		}
		control = at;
		have_control = true;
	}

	// Every contour is closed, whether or not the file says so.
	if (have_control)
		quadratic(f, pen, control, start);
	else
		emit(f, pen, start);
}

static void flatten(struct flattener *f,
		    const voe_text_truetype_outline *outline, float scale,
		    voe_math_float2 origin)
{
	uint16_t first = 0;

	f->count = 0;
	for (uint16_t c = 0; c < outline->contour_count; c++) {
		uint16_t last = outline->contour_ends[c];

		if (last >= outline->point_count || last < first) {
			first = (uint16_t)(last + 1);
			continue;
		}
		flatten_contour(f, outline, first, last, scale, origin);
		first = (uint16_t)(last + 1);
	}
}

// Adds `weight` to every pixel of `row` between xa and xb, counting a pixel the
// span only partly covers as the fraction it covers. The clip to the row is here
// rather than at the call site, because a glyph is allowed to run outside the
// box the caller measured for it.
static void add_span(float *row, uint16_t width, float xa, float xb,
		     float weight)
{
	uint32_t a;
	uint32_t b;

	if (xa < 0.0f)
		xa = 0.0f;
	if (xb > (float)width)
		xb = (float)width;
	if (xb <= xa)
		return;

	a = (uint32_t)xa;
	b = (uint32_t)xb;
	if (a >= width)
		return;
	if (a == b) {
		row[a] += (xb - xa) * weight;
		return;
	}

	row[a] += ((float)(a + 1) - xa) * weight;
	for (uint32_t i = a + 1; i < b && i < width; i++)
		row[i] += weight;
	if (b < width)
		row[b] += (xb - (float)b) * weight;
}

// The crossings of one height, sorted by x. Insertion sort because a scanline
// through a glyph crosses a handful of edges and an insertion sort over a
// handful beats anything with a call in it.
static uint32_t cross(const struct edge *edges, uint32_t count, float y,
		      struct crossing *out)
{
	uint32_t found = 0;

	for (uint32_t i = 0; i < count; i++) {
		const struct edge *e = &edges[i];
		float top = e->y0 < e->y1 ? e->y0 : e->y1;
		float bottom = e->y0 < e->y1 ? e->y1 : e->y0;
		struct crossing c;
		uint32_t j;

		// Half open, so a height that lands exactly on a shared end
		// point of two edges is counted once and not twice or nought
		// times. Getting this wrong is a one-pixel notch at every
		// place two edges meet.
		if (y < top || y >= bottom)
			continue;

		c.x = e->x0 + (y - e->y0) * (e->x1 - e->x0) / (e->y1 - e->y0);
		c.direction = e->y1 > e->y0 ? 1 : -1;

		for (j = found; j > 0 && out[j - 1].x > c.x; j--)
			out[j] = out[j - 1];
		out[j] = c;
		found++;
	}
	return found;
}

void voe_text_raster_fill(const voe_text_truetype_outline *outline, float scale,
			  voe_math_float2 origin, uint8_t *coverage,
			  uint16_t width, uint16_t height,
			  voe_base_arena *arena)
{
	struct flattener f = { 0 };
	struct crossing *crossings;
	float *row;
	float weight = 1.0f / (float)VOE_TEXT_RASTER_SAMPLES;

	VOE_BASE_ASSERT(outline != NULL, "nothing to fill");
	VOE_BASE_ASSERT(coverage != NULL, "nowhere to fill it into");
	VOE_BASE_ASSERT(arena != NULL,
			"the fill needs scratch: rule 11");
	VOE_BASE_ASSERT(scale > 0.0f,
			"a glyph rasterised at no size at all");

	if (width == 0 || height == 0 || outline->contour_count == 0)
		return;

	flatten(&f, outline, scale, origin);
	if (f.count == 0)
		return;

	f.capacity = f.count;
	f.edges = voe_base_arena_push(arena, sizeof *f.edges * f.capacity);
	flatten(&f, outline, scale, origin);
	VOE_BASE_ASSERT(f.count == f.capacity,
			"the counting pass and the writing pass disagree");

	crossings = voe_base_arena_push(arena, sizeof *crossings * f.capacity);
	row = voe_base_arena_push(arena, sizeof *row * width);

	for (uint16_t y = 0; y < height; y++) {
		for (uint16_t x = 0; x < width; x++)
			row[x] = 0.0f;

		for (uint32_t s = 0; s < VOE_TEXT_RASTER_SAMPLES; s++) {
			float at = (float)y +
				   ((float)s + 0.5f) /
					   (float)VOE_TEXT_RASTER_SAMPLES;
			uint32_t count = cross(f.edges, f.count, at, crossings);
			int32_t winding = 0;

			for (uint32_t i = 0; i + 1 < count; i++) {
				winding += crossings[i].direction;
				if (winding != 0)
					add_span(row, width, crossings[i].x,
						 crossings[i + 1].x, weight);
			}
		}

		for (uint16_t x = 0; x < width; x++) {
			float value = row[x];

			// The clamp is not defensive: a span that starts and
			// ends inside one pixel is added at full weight by each
			// sub-scanline that finds it, and rounding can leave the
			// total a hair above one.
			if (value <= 0.0f)
				continue;
			if (value > 1.0f)
				value = 1.0f;
			coverage[(uint32_t)y * width + x] =
				(uint8_t)(value * 255.0f + 0.5f);
		}
	}
}
