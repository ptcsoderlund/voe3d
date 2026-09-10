// The halving. See shrink.h for what this is for and why it is not mipmapping.
//
// ---- THE AVERAGE IS TAKEN IN LINEAR LIGHT, AND THAT IS THE WHOLE OF THE CARE
// IN THIS FILE ----
//
// The pixels are sRGB — they go up as VOE_RENDER_TEXTURE_COLOUR and the
// hardware decodes them — and sRGB is not a linear scale, so adding two of them
// up and halving is not the average of the two colours. Take a black texel and
// a white one, which is every edge in a wordmark:
//
//     naively:   (0 + 255) / 2               = 128, a mid grey
//     correctly: srgb(( 0.0 + 1.0 ) / 2)     = 188, a light grey
//
// 188 is the right answer. Doing it the first way makes every stroke in the
// logo heavier at every halving, and two halvings later the caption is a dark
// smudge — which reads as a bad picture rather than as arithmetic in the wrong
// space. It is the single most common mistake in image downscaling and it is
// invisible unless somebody says the number out loud, so here is the number.
//
// ALPHA IS AVERAGED AS IT STANDS, because alpha was never sRGB — it is a
// coverage, it is already linear, and putting it through the same curve would
// be a second bug covering for the first.
//
// (Both pictures here are fully opaque, so the colour of a transparent texel
// never comes into it. A picture with a real alpha edge would want the colour
// premultiplied before the average and unpremultiplied after, and this does not
// do that. It is written down rather than done because nothing here needs it —
// rule 10 — and because a reader who drops in a cut-out logo should find this
// sentence rather than a halo.)
//
// ---- IN PLACE, AND IT IS SAFE RATHER THAN LUCKY ----
//
// Each step writes over the picture it is reading. Output texel (x, y) is
// written at 4 * (y * half_width + x) and the last byte it reads is at
// 4 * ((2y + 1) * width + 2x + 1) + 3, and the second is always the larger — so
// the write pointer trails the read pointer for the whole pass and never
// catches it. That saves a second buffer the size of the picture, which at
// 4800 by 2000 is thirty-eight megabytes of the arena.
#include "shrink.h"

#include <base/assert.h>

#include <math.h>

// sRGB to linear, the piecewise definition, as a table: there are only 256
// possible inputs, so this is exact and there is no reason to call powf per
// texel. Built per call rather than kept in a static, because a table that is
// initialised on first use is state and this costs 256 powf calls once.
static void build_decode(float table[256])
{
	for (uint32_t i = 0; i < 256; i++) {
		float c = (float)i / 255.0f;

		table[i] = c <= 0.04045f
				   ? c / 12.92f
				   : powf((c + 0.055f) / 1.055f, 2.4f);
	}
}

// And back. This one has a continuous input so it cannot be a table of the same
// kind; it is called once per output texel per channel, which is a few million
// times at startup and nowhere in a frame.
static uint8_t encode(float linear)
{
	float c;

	if (linear <= 0.0031308f)
		c = linear * 12.92f;
	else
		c = 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;

	if (c < 0.0f)
		c = 0.0f;
	if (c > 1.0f)
		c = 1.0f;
	// + 0.5 to round rather than truncate: truncating loses half a code
	// value at every halving and two halvings of a picture would come out
	// measurably darker than one.
	return (uint8_t)(c * 255.0f + 0.5f);
}

// One step of the chain: every 2x2 block of `image` becomes one texel. An odd
// side is handled by clamping the second sample back onto the first, so the
// last row or column is averaged with itself and nothing is dropped.
static void halve(voe_assets_image *image, const float decode[256])
{
	uint32_t width = image->width;
	uint32_t height = image->height;
	uint32_t half_width = (width + 1u) / 2u;
	uint32_t half_height = (height + 1u) / 2u;
	uint8_t *pixels = image->pixels;

	for (uint32_t y = 0; y < half_height; y++) {
		uint32_t y0 = y * 2u;
		uint32_t y1 = y0 + 1u < height ? y0 + 1u : y0;

		for (uint32_t x = 0; x < half_width; x++) {
			uint32_t x0 = x * 2u;
			uint32_t x1 = x0 + 1u < width ? x0 + 1u : x0;
			const uint8_t *a = pixels + ((size_t)y0 * width + x0) * 4u;
			const uint8_t *b = pixels + ((size_t)y0 * width + x1) * 4u;
			const uint8_t *c = pixels + ((size_t)y1 * width + x0) * 4u;
			const uint8_t *d = pixels + ((size_t)y1 * width + x1) * 4u;
			uint8_t *out = pixels + ((size_t)y * half_width + x) * 4u;

			for (uint32_t channel = 0; channel < 3; channel++)
				out[channel] = encode((decode[a[channel]] +
						       decode[b[channel]] +
						       decode[c[channel]] +
						       decode[d[channel]]) *
						      0.25f);
			// Not through the curve. See the header.
			out[3] = (uint8_t)(((uint32_t)a[3] + b[3] + c[3] +
					    d[3] + 2u) /
					   4u);
		}
	}

	image->width = half_width;
	image->height = half_height;
}

void voe_dev_image_shrink(voe_assets_image *image, uint32_t long_side)
{
	float decode[256];

	VOE_BASE_ASSERT(image != NULL, "shrinking no picture");
	VOE_BASE_ASSERT(image->pixels != NULL, "shrinking a picture with no pixels");
	VOE_BASE_ASSERT(long_side > 0, "shrinking a picture to nothing");

	if (image->width <= long_side && image->height <= long_side)
		return;

	build_decode(decode);
	while (image->width > long_side || image->height > long_side)
		halve(image, decode);
}
