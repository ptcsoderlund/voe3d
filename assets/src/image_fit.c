// The area-average shrink behind voe_assets_image_fit (assets/image.h): the
// size, each output texel's footprint with fractional edge weights, and how
// each kind of picture turns the texels under it into one.
//
// ONE OUTPUT ROW AT A TIME. An output row sums every source row its footprint
// covers, each weighted by the part of it covered, into one row of
// FLOATS_PER_TEXEL floats per output texel; the row is then written out as
// bytes and cleared for the next. That row is the only working memory: pushed
// once, rewound before the return, never a float copy of the picture.
//
// THE WEIGHTS ARE SEPARABLE. A source texel's share is the part of its column
// covered times the part of its row covered, which is exactly the area it
// shares with the footprint, so a ratio like 3:2 is exact. Each sum is divided
// by its own weight total when it is written, so a constant picture stays that
// constant whatever the rounding of the weights.
//
// The footprint edges are worked in double from integer products, so the last
// one lands on the picture's edge exactly; the sums are float.
//
// COLOUR KEEPS THREE SUMS: rgb weighted by alpha, alpha itself, and rgb with
// the plain weight, the last read only where every covered texel is
// transparent. sRGB decoding is a 256-entry table built on the stack per call;
// encoding is the formula, once per written channel.
#include <assets/image.h>
#include <base/assert.h>

#include <math.h>
#include <string.h>

// Floats per output texel in the working row: colour's three rgb weighted by
// alpha, the alpha sum, three plain rgb, and the weight total last. Data and
// normal use the first four and the total.
#define FLOATS_PER_TEXEL 8
#define WEIGHT_TOTAL 7

// The source span one output texel or row covers: [start, stop) in source
// texels, and the whole texels [first, end) that span touches.
typedef struct {
	double start;
	double stop;
	uint32_t first;
	uint32_t end;
} footprint;

// `side` scaled by largest / longest, rounded, at least 1; the longest side
// itself comes out as `largest` exactly.
static uint32_t scaled_side(uint32_t side, uint32_t longest, uint32_t largest)
{
	const uint64_t scaled =
		((uint64_t)side * largest + longest / 2) / longest;

	return scaled < 1 ? 1 : (uint32_t)scaled;
}

static footprint footprint_of(uint32_t index, uint32_t source, uint32_t out)
{
	footprint span;

	span.start = (double)index * source / out;
	span.stop = (double)(index + 1) * source / out;
	span.first = (uint32_t)span.start;
	span.end = (uint32_t)ceil(span.stop);
	if (span.end > source)
		span.end = source;
	return span;
}

// The part of source texel `texel` inside the span.
static float covered(uint32_t texel, footprint span)
{
	const double from = span.start > texel ? span.start : texel;
	const double to = span.stop < texel + 1.0 ? span.stop : texel + 1.0;

	return to > from ? (float)(to - from) : 0.0f;
}

static float srgb_to_linear(float c)
{
	return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
}

static float linear_to_srgb(float c)
{
	return c <= 0.0031308f ? c * 12.92f
			       : 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}

// 0..1 to a byte, rounded and clamped.
static uint8_t to_byte(float unit)
{
	const float scaled = unit * 255.0f + 0.5f;

	return scaled <= 0.0f ? 0 : scaled >= 255.0f ? 255 : (uint8_t)scaled;
}

static void add_texel(float *sum, const uint8_t *texel, float weight,
		      voe_assets_image_kind kind, const float *linear)
{
	const float alpha = texel[3] / 255.0f;

	sum[WEIGHT_TOTAL] += weight;
	if (kind == VOE_ASSETS_IMAGE_COLOUR) {
		for (int c = 0; c < 3; c++) {
			sum[c] += weight * alpha * linear[texel[c]];
			sum[4 + c] += weight * linear[texel[c]];
		}
		sum[3] += weight * alpha;
		return;
	}
	for (int c = 0; c < 3; c++) {
		const float unit = texel[c] / 255.0f;

		sum[c] += weight * (kind == VOE_ASSETS_IMAGE_NORMAL ?
					    2.0f * unit - 1.0f :
					    unit);
	}
	sum[3] += weight * alpha;
}

// A normal whose sum cancels to nothing points straight out, (0, 0, 1).
static void write_texel(uint8_t *out, const float *sum,
			voe_assets_image_kind kind)
{
	const float total = sum[WEIGHT_TOTAL];

	out[3] = to_byte(sum[3] / total);
	if (kind == VOE_ASSETS_IMAGE_COLOUR) {
		const bool seen = sum[3] > 0.0f;

		for (int c = 0; c < 3; c++)
			out[c] = to_byte(linear_to_srgb(
				seen ? sum[c] / sum[3] : sum[4 + c] / total));
	} else if (kind == VOE_ASSETS_IMAGE_NORMAL) {
		const float length = sqrtf(sum[0] * sum[0] + sum[1] * sum[1] +
					   sum[2] * sum[2]);
		float normal[3] = { 0.0f, 0.0f, 1.0f };

		for (int c = 0; c < 3 && length > 0.0f; c++)
			normal[c] = sum[c] / length;
		for (int c = 0; c < 3; c++)
			out[c] = to_byte(normal[c] * 0.5f + 0.5f);
	} else {
		for (int c = 0; c < 3; c++)
			out[c] = to_byte(sum[c] / total);
	}
}

// One source row, already weighted by the part of it the output row covers,
// added into every output texel's sums.
static void add_row(float *sums, const uint8_t *row, uint32_t width,
		    uint32_t out_width, float row_weight,
		    voe_assets_image_kind kind, const float *linear)
{
	for (uint32_t ox = 0; ox < out_width; ox++) {
		const footprint span = footprint_of(ox, width, out_width);

		for (uint32_t sx = span.first; sx < span.end; sx++)
			add_texel(sums + (size_t)ox * FLOATS_PER_TEXEL,
				  row + (size_t)sx * 4,
				  row_weight * covered(sx, span), kind, linear);
	}
}

voe_assets_image voe_assets_image_fit(voe_base_arena *arena,
				      voe_assets_image image, uint32_t largest,
				      voe_assets_image_kind kind)
{
	const uint32_t longest =
		image.width > image.height ? image.width : image.height;
	voe_assets_image out;
	struct voe_base_arena_mark mark;
	float linear[256];
	float *sums;
	size_t row_size;

	VOE_BASE_ASSERT(largest >= 1, "a picture fitted to no size");
	VOE_BASE_ASSERT(image.pixels != NULL, "a picture with no pixels fitted");
	if (longest <= largest)
		return image;

	out.width = scaled_side(image.width, longest, largest);
	out.height = scaled_side(image.height, longest, largest);
	out.pixels = voe_base_arena_push(arena,
					 (size_t)out.width * out.height * 4);
	for (int i = 0; i < 256; i++)
		linear[i] = srgb_to_linear(i / 255.0f);

	mark = voe_base_arena_mark(arena);
	row_size = (size_t)out.width * FLOATS_PER_TEXEL * sizeof(float);
	sums = voe_base_arena_push(arena, row_size);
	for (uint32_t oy = 0; oy < out.height; oy++) {
		const footprint span = footprint_of(oy, image.height, out.height);

		memset(sums, 0, row_size);
		for (uint32_t sy = span.first; sy < span.end; sy++)
			add_row(sums,
				image.pixels + (size_t)sy * image.width * 4,
				image.width, out.width, covered(sy, span), kind,
				linear);
		for (uint32_t ox = 0; ox < out.width; ox++)
			write_texel(out.pixels +
					    ((size_t)oy * out.width + ox) * 4,
				    sums + (size_t)ox * FLOATS_PER_TEXEL, kind);
	}
	voe_base_arena_rewind(arena, mark);
	return out;
}
