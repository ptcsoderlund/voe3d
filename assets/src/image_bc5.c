// BC5, one 4×4 block both ways, as declared in image_blocks.h: red's BC4
// block in bytes 0–7, green's in bytes 8–15.
//
// A BC4 BLOCK. Byte 0 is e0, byte 1 is e1, then 48 bits read from bit 0 of
// byte 2 up: sixteen 3-bit indices in texel order. Index 0 is e0 and 1 is e1.
// When e0 > e1 indices 2–7 are the six levels between them, ((8 − i)·e0 +
// (i − 1)·e1) / 7; otherwise 2–5 are the four between, ((6 − i)·e0 +
// (i − 1)·e1) / 5, 6 is 0 and 7 is 255. A level is rounded to the nearest
// byte, as a card's UNORM conversion does.
//
// THE ENCODE, per channel. Both modes are tried and the lesser squared error
// is written: eight levels with the channel's extent as endpoints, and six
// with the extent of the texels that are not 0 or 255, which those two
// indices hold exactly. Each texel takes its nearest level, the lower index
// on a tie. A channel of one value has no eight-level pair (e0 must exceed
// e1) and is the six-level one, exact.
//
// CONSTRAINTS. Endpoints are the extent alone, never refitted: a block whose
// values bunch at one end has levels it does not use (a least-squares refit,
// as BC7 has, would lift it). No allocation, no state; every loop is fixed,
// so the same texels always give the same bytes.
#include "image_blocks.h"

#include <base/assert.h>

#include <stddef.h>

#define TEXELS 16

// One channel's block before it is packed: endpoints, indices, error.
struct channel_block {
	uint8_t end[2];
	uint8_t index[TEXELS];
	uint32_t error;
};

static uint8_t mix(uint32_t a, uint8_t e0, uint32_t b, uint8_t e1, uint32_t d)
{
	return (uint8_t)((a * e0 + b * e1 + d / 2) / d);
}

// The eight values a block with endpoints e0, e1 decodes its indices to.
static void block_levels(uint8_t e0, uint8_t e1, uint8_t level[8])
{
	level[0] = e0;
	level[1] = e1;
	if (e0 > e1) {
		for (uint32_t i = 2; i < 8; i++)
			level[i] = mix(8 - i, e0, i - 1, e1, 7);
		return;
	}
	for (uint32_t i = 2; i < 6; i++)
		level[i] = mix(6 - i, e0, i - 1, e1, 5);
	level[6] = 0;
	level[7] = 255;
}

static void index_nearest(const uint8_t value[TEXELS], struct channel_block *b)
{
	uint8_t level[8];

	block_levels(b->end[0], b->end[1], level);
	b->error = 0;
	for (int t = 0; t < TEXELS; t++) {
		uint32_t best = UINT32_MAX;

		for (uint8_t i = 0; i < 8; i++) {
			const int d = (int)level[i] - value[t];

			if ((uint32_t)(d * d) < best) {
				best = (uint32_t)(d * d);
				b->index[t] = i;
			}
		}
		b->error += best;
	}
}

static void encode_channel(const uint8_t rgba[64], int channel,
			   uint8_t block[8])
{
	uint8_t value[TEXELS], low = 255, high = 0, inner_low = 255,
			       inner_high = 0;

	for (int t = 0; t < TEXELS; t++) {
		const uint8_t v = rgba[t * 4 + channel];

		value[t] = v;
		low = v < low ? v : low;
		high = v > high ? v : high;
		if (v != 0 && v != 255) {
			inner_low = v < inner_low ? v : inner_low;
			inner_high = v > inner_high ? v : inner_high;
		}
	}
	if (inner_low > inner_high)
		inner_low = inner_high = 0;
	struct channel_block six = { .end = { inner_low, inner_high } };
	struct channel_block eight = { .end = { high, low },
				       .error = UINT32_MAX };

	index_nearest(value, &six);
	if (high > low)
		index_nearest(value, &eight);
	const struct channel_block *best = eight.error < six.error ? &eight :
								      &six;
	uint64_t bits = 0;

	for (int t = 0; t < TEXELS; t++)
		bits |= (uint64_t)best->index[t] << (3 * t);
	block[0] = best->end[0];
	block[1] = best->end[1];
	for (int i = 0; i < 6; i++)
		block[2 + i] = (uint8_t)(bits >> (8 * i));
}

static void decode_channel(const uint8_t block[8], int channel,
			   uint8_t rgba[64])
{
	uint8_t level[8];
	uint64_t bits = 0;

	block_levels(block[0], block[1], level);
	for (int i = 0; i < 6; i++)
		bits |= (uint64_t)block[2 + i] << (8 * i);
	for (int t = 0; t < TEXELS; t++)
		rgba[t * 4 + channel] = level[bits >> (3 * t) & 7u];
}

void voe_assets_bc5_encode(const uint8_t rgba[64], uint8_t block[16])
{
	VOE_BASE_ASSERT(rgba != NULL && block != NULL,
			"BC5: encoding needs texels and a block to write");
	encode_channel(rgba, 0, block);
	encode_channel(rgba, 1, block + 8);
}

void voe_assets_bc5_decode(const uint8_t block[16], uint8_t rgba[64])
{
	VOE_BASE_ASSERT(block != NULL && rgba != NULL,
			"BC5: decoding needs a block and texels to write");
	for (int t = 0; t < TEXELS; t++) {
		rgba[t * 4 + 2] = 0;
		rgba[t * 4 + 3] = 255;
	}
	decode_channel(block, 0, rgba);
	decode_channel(block + 8, 1, rgba);
}
