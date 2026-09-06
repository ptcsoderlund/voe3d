// The fill and the field, on one flattener. Two passes over the outline — one to
// count the line segments the curves become and one to write them — then one
// pass per row. See raster.h for the fill rule, the three channels, the
// anti-aliasing and where the Y flip is.
//
// TWO PASSES BECAUSE THE ARRAY IS PUSHED ONCE. How many segments a curve becomes
// depends on how curved it is in pixels, so the count is not known until the
// flattening has been worked out; and an arena's pushes are not guaranteed to be
// next to each other (base/arena.h), so growing an array by pushing again is not
// available. Counting first is the cheaper half of the same arithmetic.
//
// THE FILL IS FIRST AND THE FIELD IS AT THE BOTTOM, AND THEY SHARE THE MIDDLE.
// The flattening, the crossings and the winding rule belong to both; what the
// field adds is a colouring of the outline's edges and a distance loop, and it
// asks the fill's own winding rule for the one thing it cannot get from a
// one-sided distance.
#include "raster.h"

#include <base/assert.h>

#include <math.h>
#include <string.h>

// One flattened edge, in bitmap pixels.
//
// `outline_edge` IS WHICH `glyf` EDGE THIS PIECE CAME FROM, AND IT IS WHAT MAKES
// THE COLOURING POSSIBLE. A quadratic becomes a dozen of these and they all
// carry the same number, so the corner test can ask about the join between one
// edge of the outline and the next rather than about the join between two pieces
// of one curve — which turns by tens of degrees and is not a corner at all. See
// VOE_TEXT_FIELD_CORNER_DEGREES.
//
// `channels` IS THE FIELD'S AND IS ZERO FOR THE FILL. It is a bit per channel,
// so an edge belongs to two of the three; the fill never looks at it.
struct edge {
	float x0;
	float y0;
	float x1;
	float y1;
	uint32_t contour;
	uint32_t outline_edge;
	uint8_t channels;
};

// A scanline crossing: where an edge cuts this height, and which way it was
// going. The direction is the whole of the non-zero winding rule.
struct crossing {
	float x;
	int32_t direction;
};

// The state the two passes share. In counting mode `edges` is NULL and only
// `count` moves.
//
// `keep_flat` IS THE ONE THING THE TWO CALLERS DISAGREE ABOUT. The fill drops
// horizontal edges: they cross no scanline and carry no winding, and dropping
// them is what keeps a zero denominator out of the innermost loop below. The
// field must keep them — a flat-cut terminal is a horizontal edge and it is
// exactly the sort of edge whose distance matters.
struct flattener {
	struct edge *edges;
	uint32_t count;
	uint32_t capacity;
	bool keep_flat;
	uint32_t contour;
	uint32_t outline_edge;
};

static void emit(struct flattener *f, voe_math_float2 a, voe_math_float2 b)
{
	// A piece of no length has no direction, so neither caller can use one.
	if (a.x == b.x && a.y == b.y)
		return;
	if (!f->keep_flat && a.y == b.y)
		return;
	if (f->edges != NULL) {
		VOE_BASE_ASSERT(f->count < f->capacity,
				"the counting pass and the writing pass disagree");
		f->edges[f->count] = (struct edge){
			a.x, a.y, b.x, b.y, f->contour, f->outline_edge, 0
		};
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

		// One edge of the outline per pass through here, whether it comes
		// out as one line or as a dozen pieces of a curve. The counter
		// runs across the whole outline and not per contour, so an edge
		// number names an edge and nothing has to be paired with a
		// contour to be unique.
		if (p->on_curve) {
			if (have_control) {
				quadratic(f, pen, control, at);
				have_control = false;
			} else {
				emit(f, pen, at);
			}
			f->outline_edge++;
			pen = at;
			continue;
		}

		if (have_control) {
			voe_math_float2 implied = { (control.x + at.x) * 0.5f,
						    (control.y + at.y) * 0.5f };

			quadratic(f, pen, control, implied);
			f->outline_edge++;
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
	f->outline_edge++;
}

static void flatten(struct flattener *f,
		    const voe_text_truetype_outline *outline, float scale,
		    voe_math_float2 origin)
{
	uint16_t first = 0;

	f->count = 0;
	f->outline_edge = 0;
	for (uint16_t c = 0; c < outline->contour_count; c++) {
		uint16_t last = outline->contour_ends[c];

		if (last >= outline->point_count || last < first) {
			first = (uint16_t)(last + 1);
			continue;
		}
		f->contour = c;
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

// ---------------------------------------------------------------- the field
//
// The three distances, worked out from the same flattened segments the fill
// walks. See raster.h for what the three are for and where the sign comes from;
// this is how it is built.
//
// THE COLOURING IS THE HALF THAT FAILS QUIETLY. A mis-assigned corner is a small
// notch in one letter and nothing that fails, so the order below is written to be
// read: the flattened pieces are grouped back into the outline edges they came
// from, the joins between those edges are tested for corners, each stretch
// between two corners is given a colour, and only then does anything measure a
// distance.

// An edge belongs to two of the three channels, and the two edges meeting at a
// corner are given colours that do not share both. Sharing exactly one is what
// keeps a smooth join continuous in the median while a corner is not.
#define FIELD_YELLOW 0x3u
#define FIELD_CYAN 0x6u
#define FIELD_MAGENTA 0x5u

// All three, for a contour with no corner in it at all — an `o`, an `0`. Every
// channel then measures the same edges, the median is simply the distance, and
// there is no corner for it to have preserved.
#define FIELD_WHITE 0x7u

// The order stretches are coloured in, cycling. Any order works; what matters is
// that consecutive stretches differ, which run_colour below is responsible for.
static const uint8_t FIELD_COLOURS[3] = { FIELD_YELLOW, FIELD_CYAN,
					  FIELD_MAGENTA };

// Nothing here divides by a length without this under it. A degenerate segment
// is dropped at emit(), so this guards the arithmetic and not the data.
#define FIELD_LEAST 1e-6f

// How close two distances have to be to count as the same, in texels. Two edges
// meeting at a point are exactly equidistant from anything on the far side of
// it, and in floating point "exactly" is worth a thousandth of a texel of slack.
#define FIELD_TIE 1e-3f

// One edge of the outline, as the colouring sees it: the run of flattened pieces
// it became and the directions it starts and ends with. A quadratic's start and
// end directions differ; a line's are the same.
struct outline_edge {
	uint32_t first;
	uint32_t count;
	uint32_t contour;
	voe_math_float2 in;
	voe_math_float2 out;
};

// One flattened piece, with everything the distance loop would otherwise work
// out again for every texel.
struct field_segment {
	float ax;
	float ay;
	float dx;
	float dy;
	float inv_length;
	float inv_length2;
	uint8_t channels;
};

static voe_math_float2 unit(float dx, float dy)
{
	float length = sqrtf(dx * dx + dy * dy);

	if (length < FIELD_LEAST)
		return (voe_math_float2){ 0.0f, 0.0f };
	return (voe_math_float2){ dx / length, dy / length };
}

// The middle of three. The same function the shader has, for the same reason,
// and the two have to agree about what the sheet means.
static float median(float a, float b, float c)
{
	float low = a < b ? a : b;
	float high = a < b ? b : a;

	return high < c ? high : (low > c ? low : c);
}

// The flattened pieces, grouped back into the edges of the outline they came
// from. They arrive in order and an edge's pieces are next to each other, so
// this is one walk and not a sort.
static uint32_t collect_outline_edges(const struct edge *edges, uint32_t count,
				      struct outline_edge *out)
{
	uint32_t found = 0;

	for (uint32_t i = 0; i < count;) {
		uint32_t j = i;

		while (j < count && edges[j].outline_edge == edges[i].outline_edge)
			j++;

		out[found] = (struct outline_edge){
			.first = i,
			.count = j - i,
			.contour = edges[i].contour,
			.in = unit(edges[i].x1 - edges[i].x0,
				   edges[i].y1 - edges[i].y0),
			.out = unit(edges[j - 1].x1 - edges[j - 1].x0,
				    edges[j - 1].y1 - edges[j - 1].y0),
		};
		found++;
		i = j;
	}
	return found;
}

// Whether the turn from `a` to `b` is a corner. `corner_sine` is the sine of
// VOE_TEXT_FIELD_CORNER_DEGREES.
//
// THE DOT PRODUCT IS NOT REDUNDANT WITH THE CROSS PRODUCT. A sine cannot tell a
// turn of ten degrees from one of a hundred and seventy, so a hairpin — which is
// as sharp a corner as there is — would read as very nearly straight. The dot
// product going negative is what catches every turn past a right angle.
static bool is_corner(voe_math_float2 a, voe_math_float2 b, float corner_sine)
{
	float dot = a.x * b.x + a.y * b.y;
	float cross = a.x * b.y - a.y * b.x;

	return dot <= 0.0f || fabsf(cross) > corner_sine;
}

static bool starts_a_corner(const struct outline_edge *edges, uint32_t count,
			    uint32_t at, float corner_sine)
{
	return is_corner(edges[(at + count - 1) % count].out, edges[at].in,
			 corner_sine);
}

// Which colour a stretch gets. Cycling through three, except that the ring
// closes: the last stretch touches the first, and cycling leaves those two the
// same whenever the number of stretches is one more than a multiple of three.
// The last one then takes the colour that differs from both its neighbours,
// which with three colours and two constraints is the one that is left.
static uint8_t run_colour(uint32_t run, uint32_t runs)
{
	if (runs >= 3 && runs % 3 == 1 && run == runs - 1)
		return FIELD_COLOURS[1];
	return FIELD_COLOURS[run % 3];
}

// One contour coloured, and its colours written through onto the flattened
// pieces. `run_of` is scratch, one entry per edge of this contour.
static void colour_contour(const struct outline_edge *edges, uint32_t count,
			   struct edge *segments, float corner_sine,
			   uint32_t *run_of)
{
	uint32_t first_corner = count;
	uint32_t runs = 0;

	for (uint32_t i = 0; i < count; i++) {
		if (!starts_a_corner(edges, count, i, corner_sine))
			continue;
		if (first_corner == count)
			first_corner = i;
		runs++;
	}

	// Smooth the whole way round. Every channel measures every edge, the
	// median is the plain distance, and this is the right answer rather than
	// a fallback: there is no corner in an `o` to preserve.
	if (runs == 0) {
		for (uint32_t i = 0; i < count; i++) {
			for (uint32_t k = 0; k < edges[i].count; k++)
				segments[edges[i].first + k].channels =
					FIELD_WHITE;
		}
		return;
	}

	if (runs >= 2) {
		uint32_t run = 0;

		for (uint32_t k = 0; k < count; k++) {
			uint32_t i = (first_corner + k) % count;

			if (k > 0 &&
			    starts_a_corner(edges, count, i, corner_sine))
				run++;
			run_of[i] = run;
		}
	} else {
		// A teardrop: one corner, and the single stretch runs all the
		// way round back to it. Colouring that stretch one colour would
		// put the same colour on both sides of the corner and round it
		// off, so it is cut into three by edge count instead. The two
		// cuts inside the stretch are invisible — the colours either
		// side of one share a channel and the median is continuous
		// across it.
		runs = count >= 3 ? 3 : count;
		for (uint32_t k = 0; k < count; k++) {
			uint32_t i = (first_corner + k) % count;

			run_of[i] = k * runs / count;
		}
	}

	for (uint32_t i = 0; i < count; i++) {
		uint8_t colour = run_colour(run_of[i], runs);

		for (uint32_t k = 0; k < edges[i].count; k++)
			segments[edges[i].first + k].channels = colour;
	}
}

static void colour_outline(const struct outline_edge *edges, uint32_t count,
			   struct edge *segments, float corner_sine,
			   uint32_t *run_of)
{
	for (uint32_t i = 0; i < count;) {
		uint32_t j = i;

		while (j < count && edges[j].contour == edges[i].contour)
			j++;
		colour_contour(edges + i, j - i, segments, corner_sine,
			       run_of + i);
		i = j;
	}
}

// How one segment reaches a texel centre: how far away it is, how squarely it
// faces it, and where the line it lies on puts it.
//
// THE VALUE THAT ENDS UP IN THE SHEET IS THE DISTANCE TO THE *LINE* AND NOT TO
// THE SEGMENT, AND THAT IS THE WHOLE OF WHY A CORNER STAYS SHARP. Past the end
// of an edge, the segment's own distance is the distance to its end point, which
// is a circle — three circles meeting is a rounded corner. Its line carries
// straight on instead, so on the far side of a corner one channel says inside
// and another says outside, and the median lands on the crossing of the two
// lines, which is the corner. `orthogonality` is what decides which of two edges
// meeting at a point gets to extend: the one more squarely on is the one whose
// line is the shorter reach and therefore the honest one.
static void reach_of(const struct field_segment *segment, float px, float py,
		     float *distance, float *orthogonality, float *signed_line)
{
	float rx = px - segment->ax;
	float ry = py - segment->ay;
	float t = (rx * segment->dx + ry * segment->dy) * segment->inv_length2;
	float clamped = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	float ex = rx - clamped * segment->dx;
	float ey = ry - clamped * segment->dy;
	float away = sqrtf(ex * ex + ey * ey);

	*distance = away;
	// Positive inside. The sign is the cross product of the edge's own
	// direction with the way to the texel, which is exactly the fact the
	// non-zero winding rule counts — see raster.h.
	*signed_line = (segment->dx * ry - segment->dy * rx) *
		       segment->inv_length;
	*orthogonality =
		t == clamped ?
			1.0f :
			fabsf(segment->dx * ey - segment->dy * ex) *
				segment->inv_length /
				(away > FIELD_LEAST ? away : FIELD_LEAST);
}

// A signed distance in texels, as the byte the sheet holds: nothing at
// VOE_TEXT_FIELD_SPREAD outside, everything at that far inside, and the halfway
// mark exactly on the outline.
static uint8_t encode(float distance)
{
	float at = 0.5f + 0.5f * distance / VOE_TEXT_FIELD_SPREAD;

	if (at <= 0.0f)
		return 0;
	if (at >= 1.0f)
		return 255;
	return (uint8_t)(at * 255.0f + 0.5f);
}

// The three values for one texel, before they are encoded. `nearest` comes back
// as the plain distance to the whole outline, which is what the sign check needs
// when the three disagree with the fill rule.
static void field_at(const struct field_segment *segments, uint32_t count,
		     float px, float py, float *values, float *nearest)
{
	float best[VOE_TEXT_FIELD_CHANNELS];
	float orthogonality[VOE_TEXT_FIELD_CHANNELS];

	for (uint32_t ch = 0; ch < VOE_TEXT_FIELD_CHANNELS; ch++) {
		best[ch] = INFINITY;
		orthogonality[ch] = -1.0f;
		// Fully outside, which is what a channel no edge was assigned to
		// should read as. The colouring always covers all three, so this
		// is the answer to a question nothing asks rather than a case.
		values[ch] = -VOE_TEXT_FIELD_SPREAD;
	}
	*nearest = INFINITY;

	for (uint32_t i = 0; i < count; i++) {
		float distance;
		float squareness;
		float signed_line;

		reach_of(&segments[i], px, py, &distance, &squareness,
			 &signed_line);
		if (distance < *nearest)
			*nearest = distance;

		for (uint32_t ch = 0; ch < VOE_TEXT_FIELD_CHANNELS; ch++) {
			if ((segments[i].channels & (1u << ch)) == 0)
				continue;
			// Nearer wins; equally near and more squarely on wins.
			// The second half is not a nicety: at the point two
			// edges share, both are exactly as near, and taking
			// either one's line at random is a notch.
			if (distance >= best[ch] - FIELD_TIE &&
			    (distance > best[ch] + FIELD_TIE ||
			     squareness <= orthogonality[ch]))
				continue;
			if (distance < best[ch])
				best[ch] = distance;
			orthogonality[ch] = squareness;
			values[ch] = signed_line;
		}
	}
}

// Where each texel centre of one row falls, inside the shape or outside it, by
// the non-zero winding rule. The same crossings the fill uses, at one height per
// row rather than at VOE_TEXT_RASTER_SAMPLES of them, because this is a question
// about a point and not about an area.
static void inside_row(const struct edge *edges, uint32_t count, float y,
		       struct crossing *crossings, uint16_t width,
		       uint8_t *inside)
{
	uint32_t found = cross(edges, count, y, crossings);
	int32_t winding = 0;
	uint32_t at = 0;

	for (uint16_t x = 0; x < width; x++) {
		float px = (float)x + 0.5f;

		while (at < found && crossings[at].x <= px) {
			winding += crossings[at].direction;
			at++;
		}
		inside[x] = winding != 0;
	}
}

void voe_text_raster_field(const voe_text_truetype_outline *outline, float scale,
			   voe_math_float2 origin, uint8_t *field,
			   uint16_t width, uint16_t height,
			   voe_base_arena *arena)
{
	struct flattener f = { .keep_flat = true };
	struct outline_edge *outline_edges;
	struct field_segment *segments;
	struct crossing *crossings;
	uint32_t *run_of;
	uint8_t *inside;
	uint32_t outline_edge_count;
	float corner_sine = sinf(VOE_TEXT_FIELD_CORNER_DEGREES *
				 3.14159265358979f / 180.0f);

	VOE_BASE_ASSERT(outline != NULL, "nothing to measure");
	VOE_BASE_ASSERT(field != NULL, "nowhere to measure it into");
	VOE_BASE_ASSERT(arena != NULL, "the field needs scratch: rule 11");
	VOE_BASE_ASSERT(scale > 0.0f, "a glyph measured at no size at all");

	// Everything at the outside extreme, so a glyph with no outline — a
	// space — comes back as a sheet that draws nothing rather than as
	// whatever the scratch last held.
	memset(field, 0,
	       (size_t)width * height * (size_t)VOE_TEXT_FIELD_CHANNELS);

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

	// One outline edge per flattened piece is the most there can be, which
	// is the case where every edge of the glyph was already straight.
	outline_edges = voe_base_arena_push(arena,
					    sizeof *outline_edges * f.count);
	run_of = voe_base_arena_push(arena, sizeof *run_of * f.count);
	outline_edge_count = collect_outline_edges(f.edges, f.count,
						   outline_edges);
	colour_outline(outline_edges, outline_edge_count, f.edges, corner_sine,
		       run_of);

	segments = voe_base_arena_push(arena, sizeof *segments * f.count);
	for (uint32_t i = 0; i < f.count; i++) {
		float dx = f.edges[i].x1 - f.edges[i].x0;
		float dy = f.edges[i].y1 - f.edges[i].y0;
		float length = sqrtf(dx * dx + dy * dy);

		segments[i] = (struct field_segment){
			.ax = f.edges[i].x0,
			.ay = f.edges[i].y0,
			.dx = dx,
			.dy = dy,
			.inv_length = 1.0f / (length > FIELD_LEAST ? length :
							            FIELD_LEAST),
			.inv_length2 = 1.0f / (length * length > FIELD_LEAST ?
						       length * length :
						       FIELD_LEAST),
			.channels = f.edges[i].channels,
		};
	}

	crossings = voe_base_arena_push(arena, sizeof *crossings * f.count);
	inside = voe_base_arena_push(arena, width);

	for (uint16_t y = 0; y < height; y++) {
		inside_row(f.edges, f.count, (float)y + 0.5f, crossings, width,
			   inside);

		for (uint16_t x = 0; x < width; x++) {
			float values[VOE_TEXT_FIELD_CHANNELS];
			float nearest;
			float middle;
			uint8_t *texel = field + ((size_t)y * width + x) *
							 VOE_TEXT_FIELD_CHANNELS;

			field_at(segments, f.count, (float)x + 0.5f,
				 (float)y + 0.5f, values, &nearest);

			// THE ONE PLACE THE TWO HALVES OF THIS FILE MEET, AND IT
			// IS A REPAIR AND NOT THE MECHANISM. Where contours
			// overlap, three one-sided fields can agree on a side
			// the shape is not on, and what that looks like is a
			// hole or a blob in one letter. So the fill rule is
			// asked, and a median that disagrees with it by more
			// than half a texel is thrown away for the plain signed
			// distance — which gives up the corner at that one
			// texel and keeps the letter. Half a texel of slack,
			// because a texel sitting on the outline is entitled to
			// disagree by a rounding error and is exactly the texel
			// worth not touching.
			middle = median(values[0], values[1], values[2]);
			if (inside[x] && middle < -0.5f) {
				values[0] = nearest;
				values[1] = nearest;
				values[2] = nearest;
			} else if (!inside[x] && middle > 0.5f) {
				values[0] = -nearest;
				values[1] = -nearest;
				values[2] = -nearest;
			}

			texel[0] = encode(values[0]);
			texel[1] = encode(values[1]);
			texel[2] = encode(values[2]);
		}
	}
}
