// The texture encoder's compute entries (ADR-0486 point 2), dispatched by
// render/src/compress.c once per level of a chain: `bc7` writes BC7 mode 6
// blocks, `bc5` BC5 blocks, one thread a 4×4 block, 8×8 threads a group.
//
// IN, binding 0: the chain's RGBA8 texels, one word each, R in its low byte, as
// voe_render_texture_create_levels takes them. OUT, binding 1: the blocks, four
// words each, little-endian, so bit 0 of byte 0 is bit 0 of word 0, as
// voe_render_texture_create_blocks takes them. The push block names the level:
// its sides, its first texel and its first block. Block (bx, by) is at
// by × ⌈width / 4⌉ + bx from the first; a thread past the level's blocks
// returns. A texel past the level's edge is the nearest edge texel.
//
// BC7, a port of assets/src/image_bc7.c block for block: the texels' principal
// axis in RGBA by power iteration on their covariance from its row of largest
// variance, the extent along it as endpoints; each endpoint's p-bit tried both
// ways, every texel to its nearest level (the lower index on a tie), the least
// squared error kept, alpha's counting ALPHA_WEIGHT times; one least-squares
// refit tried the same way; the indices flipped when texel 0's top bit is set.
//
// BC5, a port of assets/src/image_bc5.c: red's BC4 half in bytes 0–7, green's
// in 8–15; per half both BC4 modes, eight levels over the extent and six over
// the extent of the values not 0 or 255, the lesser squared error written.
//
// CONSTRAINTS. Mode 6 alone, as the CPU encoder: partitioned modes would lift
// an edge between three colours drawn to one line. Floating point on the GPU
// may round a step differently from the CPU, so a block may differ in a bit
// from the CPU's; the bounds the CPU encoder was held to hold here. Every loop
// is fixed, so the same texels always give the same bytes.

struct compress_push {
	uint width;
	uint height;
	uint texel;
	uint block;
};

[[vk::binding(0, 0)]]
StructuredBuffer<uint> compress_texels;
[[vk::binding(1, 0)]]
RWStructuredBuffer<uint> compress_blocks;

[[vk::push_constant]]
ConstantBuffer<compress_push> compress_level;

static const uint WEIGHTS[16] = { 0,  4,  9,  13, 17, 21, 26, 30,
				  34, 38, 43, 47, 51, 55, 60, 64 };
static const uint ALPHA_WEIGHT = 8;
static const uint POWER_STEPS = 8;
// Below this the indices name one level only (image_bc7.c's REFIT_FLOOR).
static const float REFIT_FLOOR = 1e-3;

// The block's sixteen texels, a texel past the edge the nearest edge one's.
void load_texels(uint2 block, out uint4 texel[16])
{
	for (uint t = 0; t < 16; t++) {
		uint x = min(block.x * 4 + t % 4, compress_level.width - 1);
		uint y = min(block.y * 4 + t / 4, compress_level.height - 1);
		uint word = compress_texels[compress_level.texel +
					    y * compress_level.width + x];

		texel[t] = uint4(word & 0xff, (word >> 8) & 0xff,
				 (word >> 16) & 0xff, word >> 24);
	}
}

// `count` bits of `value` at bit `at`, from bit 0 of word 0 up.
void put_bits(inout uint words[4], inout uint at, uint value, uint count)
{
	uint shift = at % 32;

	words[at / 32] |= value << shift;
	if (shift + count > 32)
		words[at / 32 + 1] |= value >> (32 - shift);
	at += count;
}

void store_block(uint block, uint words[4])
{
	for (uint i = 0; i < 4; i++)
		compress_blocks[(compress_level.block + block) * 4 + i] = words[i];
}

// ---- BC7 mode 6

struct bc7_candidate {
	uint4 endpoint[2];
	uint index[16];
	uint error;
};

uint4 interpolate(uint4 e0, uint4 e1, uint weight)
{
	return ((64 - weight) * e0 + weight * e1 + 32) >> 6;
}

uint level_error(uint4 texel, uint4 e0, uint4 e1, uint weight)
{
	int4 d = int4(interpolate(e0, e1, weight)) - int4(texel);
	uint4 square = uint4(d * d);

	return square.r + square.g + square.b + square.a * ALPHA_WEIGHT;
}

void bc7_index_nearest(uint4 texel[16], inout bc7_candidate candidate)
{
	candidate.error = 0;
	for (uint t = 0; t < 16; t++) {
		uint best = 0xffffffff;

		for (uint i = 0; i < 16; i++) {
			uint error = level_error(texel[t], candidate.endpoint[0],
						 candidate.endpoint[1], WEIGHTS[i]);

			if (error < best) {
				best = error;
				candidate.index[t] = i;
			}
		}
		candidate.error += best;
	}
}

// The nearest byte 2k + p, k in 0..127.
uint4 quantise(float4 value, uint p)
{
	float4 k = clamp(floor((value - float(p)) / 2.0 + 0.5), 0.0, 127.0);

	return (uint4(k) << 1) | p;
}

void try_endpoints(uint4 texel[16], float4 ends[2], inout bc7_candidate best)
{
	for (uint pair = 0; pair < 4; pair++) {
		bc7_candidate candidate;

		for (uint e = 0; e < 2; e++)
			candidate.endpoint[e] = quantise(ends[e], (pair >> e) & 1);
		bc7_index_nearest(texel, candidate);
		if (candidate.error < best.error)
			best = candidate;
	}
}

// The texels' extent along their principal axis in RGBA; with no variance
// both ends are the mean.
void extent_endpoints(uint4 texel[16], out float4 ends[2])
{
	float4 mean = 0, covariance[4];
	float low = 0, high = 0;
	uint row = 0;

	for (uint i = 0; i < 4; i++)
		covariance[i] = 0;
	for (uint t = 0; t < 16; t++)
		mean += float4(texel[t]) / 16.0;
	for (uint t = 0; t < 16; t++) {
		float4 d = float4(texel[t]) - mean;

		for (uint i = 0; i < 4; i++)
			covariance[i] += d[i] * d;
	}
	for (uint i = 1; i < 4; i++)
		if (covariance[i][i] > covariance[row][row])
			row = i;
	float4 axis = covariance[row];
	for (uint step = 0; step < POWER_STEPS; step++) {
		float4 next;

		for (uint i = 0; i < 4; i++)
			next[i] = dot(covariance[i], axis);
		float largest = max(max(abs(next.x), abs(next.y)),
				    max(abs(next.z), abs(next.w)));
		if (largest == 0)
			break;
		axis = next / largest;
	}
	float length = sqrt(dot(axis, axis));
	axis = length > 0 ? axis / length : float4(0);
	for (uint t = 0; t < 16; t++) {
		float along = dot(float4(texel[t]) - mean, axis);

		low = min(low, along);
		high = max(high, along);
	}
	ends[0] = clamp(mean + low * axis, 0.0, 255.0);
	ends[1] = clamp(mean + high * axis, 0.0, 255.0);
}

// The endpoints that fit the candidate's indices best by least squares, or
// false when they name one level.
bool refit(uint4 texel[16], bc7_candidate candidate, inout float4 ends[2])
{
	float aa = 0, ab = 0, bb = 0;
	float4 xa = 0, xb = 0;

	for (uint t = 0; t < 16; t++) {
		float b = WEIGHTS[candidate.index[t]] / 64.0;
		float a = 1.0 - b;

		aa += a * a;
		ab += a * b;
		bb += b * b;
		xa += a * float4(texel[t]);
		xb += b * float4(texel[t]);
	}
	float det = aa * bb - ab * ab;
	if (det < REFIT_FLOOR)
		return false;
	ends[0] = clamp((bb * xa - ab * xb) / det, 0.0, 255.0);
	ends[1] = clamp((aa * xb - ab * xa) / det, 0.0, 255.0);
	return true;
}

void bc7_write(bc7_candidate best, out uint words[4])
{
	bool flip = best.index[0] >= 8;
	uint4 e0 = flip ? best.endpoint[1] : best.endpoint[0];
	uint4 e1 = flip ? best.endpoint[0] : best.endpoint[1];
	uint at = 0;

	for (uint i = 0; i < 4; i++)
		words[i] = 0;
	put_bits(words, at, 1 << 6, 7);
	for (uint c = 0; c < 4; c++) {
		put_bits(words, at, e0[c] >> 1, 7);
		put_bits(words, at, e1[c] >> 1, 7);
	}
	put_bits(words, at, e0.r & 1, 1);
	put_bits(words, at, e1.r & 1, 1);
	for (uint t = 0; t < 16; t++) {
		uint index = best.index[t];

		put_bits(words, at, flip ? 15 - index : index, t == 0 ? 3 : 4);
	}
}

[shader("compute")]
[numthreads(8, 8, 1)]
void bc7(uint3 id: SV_DispatchThreadID)
{
	uint across = (compress_level.width + 3) / 4;
	if (id.x >= across || id.y >= (compress_level.height + 3) / 4)
		return;
	uint4 texel[16];
	float4 ends[2];
	bc7_candidate best;
	uint words[4];

	load_texels(id.xy, texel);
	best.error = 0xffffffff;
	extent_endpoints(texel, ends);
	try_endpoints(texel, ends, best);
	if (refit(texel, best, ends))
		try_endpoints(texel, ends, best);
	bc7_write(best, words);
	store_block(id.y * across + id.x, words);
}

// ---- BC5, two BC4 halves

struct bc4_candidate {
	uint end[2];
	uint index[16];
	uint error;
};

uint mix(uint a, uint e0, uint b, uint e1, uint d)
{
	return (a * e0 + b * e1 + d / 2) / d;
}

// The eight values a half with endpoints e0, e1 decodes its indices to.
void bc4_levels(uint e0, uint e1, out uint level[8])
{
	level[0] = e0;
	level[1] = e1;
	if (e0 > e1) {
		for (uint i = 2; i < 8; i++)
			level[i] = mix(8 - i, e0, i - 1, e1, 7);
		return;
	}
	for (uint i = 2; i < 6; i++)
		level[i] = mix(6 - i, e0, i - 1, e1, 5);
	level[6] = 0;
	level[7] = 255;
}

void bc4_index_nearest(uint value[16], inout bc4_candidate candidate)
{
	uint level[8];

	bc4_levels(candidate.end[0], candidate.end[1], level);
	candidate.error = 0;
	for (uint t = 0; t < 16; t++) {
		uint best = 0xffffffff;

		for (uint i = 0; i < 8; i++) {
			int d = int(level[i]) - int(value[t]);

			if (uint(d * d) < best) {
				best = uint(d * d);
				candidate.index[t] = i;
			}
		}
		candidate.error += best;
	}
}

// Channel `channel`'s half, written at bit 64 × channel.
void bc4_encode(uint4 texel[16], uint channel, inout uint words[4])
{
	uint value[16];
	uint low = 255, high = 0, inner_low = 255, inner_high = 0;

	for (uint t = 0; t < 16; t++) {
		uint v = texel[t][channel];

		value[t] = v;
		low = min(low, v);
		high = max(high, v);
		if (v != 0 && v != 255) {
			inner_low = min(inner_low, v);
			inner_high = max(inner_high, v);
		}
	}
	if (inner_low > inner_high) {
		inner_low = 0;
		inner_high = 0;
	}
	bc4_candidate best, eight;
	best.end[0] = inner_low;
	best.end[1] = inner_high;
	bc4_index_nearest(value, best);
	if (high > low) {
		eight.end[0] = high;
		eight.end[1] = low;
		bc4_index_nearest(value, eight);
		if (eight.error < best.error)
			best = eight;
	}
	uint at = 64 * channel;
	put_bits(words, at, best.end[0], 8);
	put_bits(words, at, best.end[1], 8);
	for (uint t = 0; t < 16; t++)
		put_bits(words, at, best.index[t], 3);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void bc5(uint3 id: SV_DispatchThreadID)
{
	uint across = (compress_level.width + 3) / 4;
	if (id.x >= across || id.y >= (compress_level.height + 3) / 4)
		return;
	uint4 texel[16];
	uint words[4] = { 0, 0, 0, 0 };

	load_texels(id.xy, texel);
	bc4_encode(texel, 0, words);
	bc4_encode(texel, 1, words);
	store_block(id.y * across + id.x, words);
}
