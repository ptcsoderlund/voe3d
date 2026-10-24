// BC7 mode 6, one 4×4 block both ways, as declared in image_blocks.h.
//
// THE BLOCK. 128 bits read from bit 0 of byte 0 up: the mode, six zeros and a
// one; seven bits each of R0 R1 G0 G1 B0 B1 A0 A1; the p-bits P0 and P1; then
// sixteen 4-bit indices in texel order, the first with its top bit left out
// and taken as 0. An endpoint's channel is its seven bits with its p-bit
// below them, eight bits with nothing further expanded, and a texel's channel
// is ((64 - w) * e0 + w * e1 + 32) >> 6, alpha included, w being BC7's 4-bit
// weight for its index.
//
// THE ENCODE. The axis is the texels' principal one in RGBA, by power
// iteration on their covariance from its row of largest variance; the
// endpoints are the texels' extent along it, clamped to a byte. A block of one
// colour has no axis, so both endpoints are that colour, exact but for the
// p-bit. Each endpoint's p-bit is tried both ways, the four pairs quantised,
// every texel given its nearest level, and the pair with the least error is
// the candidate. One least-squares refit of the endpoints from its indices
// makes a second candidate the same way, and the lesser error is written.
// When texel 0's index has its top bit set, the endpoints swap and every index
// flips, which BC7's symmetric weights make the same block.
//
// THE ERROR is squared, alpha's counting ALPHA_WEIGHT times a colour
// channel's. A p-bit is shared by an endpoint's four channels; unweighted, an
// opaque block would trade alpha 255 for 254 to spare three colour channels a
// step of one. Weighted, alpha's 0 and 255 stay exact beside opaque texels and
// the colour takes the step.
//
// CONSTRAINTS. Mode 6 alone: texels off one line in RGBA, an edge between
// three colours, are drawn to the line (0477 point 2; partitioned modes would
// lift it). No allocation, no state; every step is a fixed loop, so the same
// texels always give the same bytes.
#include "image_blocks.h"

#include <base/assert.h>

#include <math.h>
#include <stdbool.h>
#include <string.h>

#define TEXELS 16
#define ALPHA_WEIGHT 8
#define POWER_STEPS 8
// Below this the indices name one level only; the smallest real value, one
// texel a level from fifteen others, is 15/256.
#define REFIT_FLOOR 1e-3f

static const uint8_t WEIGHTS[16] = { 0,	 4,  9,	 13, 17, 21, 26, 30,
				     34, 38, 43, 47, 51, 55, 60, 64 };

// Endpoints as stored, p-bit the low bit of every channel, and each texel's
// index into the sixteen levels between them.
struct candidate {
	uint8_t endpoint[2][4];
	uint8_t index[TEXELS];
	uint32_t error;
};

static uint8_t interpolate(uint8_t e0, uint8_t e1, uint8_t weight)
{
	return (uint8_t)(((64u - weight) * e0 + (uint32_t)weight * e1 + 32u) >>
			 6);
}

static float to_byte_range(float value)
{
	return fminf(fmaxf(value, 0.0f), 255.0f);
}

static uint32_t level_error(const uint8_t *texel, const uint8_t *e0,
			    const uint8_t *e1, uint8_t weight)
{
	uint32_t error = 0;

	for (int c = 0; c < 4; c++) {
		const int d = (int)interpolate(e0[c], e1[c], weight) - texel[c];

		error += (uint32_t)(d * d) * (c == 3 ? ALPHA_WEIGHT : 1u);
	}
	return error;
}

// Every texel to its nearest level, the lower index on a tie; the error summed.
static void index_nearest(const uint8_t rgba[64], struct candidate *candidate)
{
	candidate->error = 0;
	for (int t = 0; t < TEXELS; t++) {
		uint32_t best = UINT32_MAX;

		for (uint8_t i = 0; i < 16; i++) {
			const uint32_t error = level_error(
				rgba + t * 4, candidate->endpoint[0],
				candidate->endpoint[1], WEIGHTS[i]);

			if (error < best) {
				best = error;
				candidate->index[t] = i;
			}
		}
		candidate->error += best;
	}
}

// The nearest byte 2k + p, k in 0..127.
static uint8_t quantise(float value, uint8_t p)
{
	const float k = fminf(fmaxf(floorf((value - p) / 2.0f + 0.5f), 0.0f),
			      127.0f);

	return (uint8_t)((uint8_t)k << 1 | p);
}

// `ends` quantised with each of the four p-bit pairs and indexed; the best
// kept in `best` if it beats what is there.
static void try_endpoints(const uint8_t rgba[64], const float ends[2][4],
			  struct candidate *best)
{
	for (uint8_t pair = 0; pair < 4; pair++) {
		struct candidate candidate = { 0 };

		for (int e = 0; e < 2; e++)
			for (int c = 0; c < 4; c++)
				candidate.endpoint[e][c] = quantise(
					ends[e][c], (uint8_t)(pair >> e & 1u));
		index_nearest(rgba, &candidate);
		if (candidate.error < best->error)
			*best = candidate;
	}
}

// The texels' extent along their principal axis in RGBA. With no variance
// the axis stays zero and both ends are the mean, the block's one colour.
static void extent_endpoints(const uint8_t rgba[64], float ends[2][4])
{
	float mean[4] = { 0 }, covariance[4][4] = { 0 }, axis[4], length = 0;
	float low = 0, high = 0;
	int row = 0;

	for (int t = 0; t < TEXELS; t++)
		for (int c = 0; c < 4; c++)
			mean[c] += rgba[t * 4 + c] / (float)TEXELS;
	for (int t = 0; t < TEXELS; t++)
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				covariance[i][j] += (rgba[t * 4 + i] - mean[i]) *
						    (rgba[t * 4 + j] - mean[j]);
	for (int i = 1; i < 4; i++)
		if (covariance[i][i] > covariance[row][row])
			row = i;
	memcpy(axis, covariance[row], sizeof axis);
	for (int step = 0; step < POWER_STEPS; step++) {
		float next[4] = { 0 }, largest = 0;

		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				next[i] += covariance[i][j] * axis[j];
		for (int i = 0; i < 4; i++)
			largest = fmaxf(largest, fabsf(next[i]));
		if (largest == 0)
			break;
		for (int i = 0; i < 4; i++)
			axis[i] = next[i] / largest;
	}
	for (int i = 0; i < 4; i++)
		length += axis[i] * axis[i];
	length = sqrtf(length);
	for (int i = 0; i < 4; i++)
		axis[i] = length > 0 ? axis[i] / length : 0;
	for (int t = 0; t < TEXELS; t++) {
		float along = 0;

		for (int c = 0; c < 4; c++)
			along += (rgba[t * 4 + c] - mean[c]) * axis[c];
		low = fminf(low, along);
		high = fmaxf(high, along);
	}
	for (int c = 0; c < 4; c++) {
		ends[0][c] = to_byte_range(mean[c] + low * axis[c]);
		ends[1][c] = to_byte_range(mean[c] + high * axis[c]);
	}
}

// The endpoints that fit `candidate`'s indices best by least squares, or
// false when the indices name one level and there is nothing to solve.
static bool refit(const uint8_t rgba[64], const struct candidate *candidate,
		  float ends[2][4])
{
	float aa = 0, ab = 0, bb = 0, xa[4] = { 0 }, xb[4] = { 0 };

	for (int t = 0; t < TEXELS; t++) {
		const float b = WEIGHTS[candidate->index[t]] / 64.0f;
		const float a = 1.0f - b;

		aa += a * a;
		ab += a * b;
		bb += b * b;
		for (int c = 0; c < 4; c++) {
			xa[c] += a * rgba[t * 4 + c];
			xb[c] += b * rgba[t * 4 + c];
		}
	}
	const float det = aa * bb - ab * ab;

	if (det < REFIT_FLOOR)
		return false;
	for (int c = 0; c < 4; c++) {
		ends[0][c] = to_byte_range((bb * xa[c] - ab * xb[c]) / det);
		ends[1][c] = to_byte_range((aa * xb[c] - ab * xa[c]) / det);
	}
	return true;
}

// `count` bits of `value` at bit `*at`, from bit 0 of byte 0 up.
static void put_bits(uint8_t *block, uint32_t *at, uint32_t value,
		     uint32_t count)
{
	for (uint32_t i = 0; i < count; i++, (*at)++)
		if (value >> i & 1u)
			block[*at / 8] |= (uint8_t)(1u << (*at % 8));
}

static uint32_t get_bits(const uint8_t *block, uint32_t *at, uint32_t count)
{
	uint32_t value = 0;

	for (uint32_t i = 0; i < count; i++, (*at)++)
		value |= (uint32_t)(block[*at / 8] >> (*at % 8) & 1u) << i;
	return value;
}

static void write_block(const struct candidate *candidate, uint8_t block[16])
{
	const bool flip = candidate->index[0] >= 8;
	const uint8_t *e0 = candidate->endpoint[flip ? 1 : 0];
	const uint8_t *e1 = candidate->endpoint[flip ? 0 : 1];
	uint32_t at = 0;

	memset(block, 0, 16);
	put_bits(block, &at, 1u << 6, 7);
	for (int c = 0; c < 4; c++) {
		put_bits(block, &at, e0[c] >> 1, 7);
		put_bits(block, &at, e1[c] >> 1, 7);
	}
	put_bits(block, &at, e0[0] & 1u, 1);
	put_bits(block, &at, e1[0] & 1u, 1);
	for (int t = 0; t < TEXELS; t++) {
		const uint8_t index = candidate->index[t];

		put_bits(block, &at, flip ? 15u - index : index, t == 0 ? 3 : 4);
	}
	VOE_BASE_ASSERT(at == 128, "BC7: a mode 6 block is 128 bits");
}

void voe_assets_bc7_encode(const uint8_t rgba[64], uint8_t block[16])
{
	VOE_BASE_ASSERT(rgba != NULL && block != NULL,
			"BC7: encoding needs texels and a block to write");
	struct candidate best = { .error = UINT32_MAX };
	float ends[2][4];

	extent_endpoints(rgba, ends);
	try_endpoints(rgba, ends, &best);
	if (refit(rgba, &best, ends))
		try_endpoints(rgba, ends, &best);
	write_block(&best, block);
}

void voe_assets_bc7_decode(const uint8_t block[16], uint8_t rgba[64])
{
	VOE_BASE_ASSERT(block != NULL && rgba != NULL,
			"BC7: decoding needs a block and texels to write");
	uint8_t ends[2][4];
	uint32_t at = 0;
	const uint32_t mode = get_bits(block, &at, 7);

	VOE_BASE_ASSERT(mode == 1u << 6,
			"BC7: only mode 6 is decoded, the one mode encoded");
	for (int c = 0; c < 4; c++)
		for (int e = 0; e < 2; e++)
			ends[e][c] = (uint8_t)(get_bits(block, &at, 7) << 1);
	for (int e = 0; e < 2; e++) {
		const uint8_t p = (uint8_t)get_bits(block, &at, 1);

		for (int c = 0; c < 4; c++)
			ends[e][c] |= p;
	}
	for (int t = 0; t < TEXELS; t++) {
		const uint8_t weight = WEIGHTS[get_bits(block, &at, t == 0 ? 3 : 4)];

		for (int c = 0; c < 4; c++)
			rgba[t * 4 + c] =
				interpolate(ends[0][c], ends[1][c], weight);
	}
}
