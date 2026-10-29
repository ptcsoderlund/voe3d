// The cloud field's two baked textures (see clouds_noise.h for what and why).
//
// Integer arithmetic only: positions are Q16 fractions of a cell (ONE is 1) or
// whole fine units, 256 to a texel, never a float, so no compiler's
// contraction and no libm can change a byte between machines. The weather is
// evaluated twice, once for its lowest and highest value and once to write it
// stretched over 0 to 255, so it uses the whole byte whatever gradient noise's
// own range. The detail is evaluated once, its sum over its greatest possible
// value: its own range already spans most of the byte. Nothing is allocated
// beyond the detail's feature points, 56 kB on the stack.
//
// THE WEATHER is gradient noise over five octaves of 4 to 64 lattice cells a
// side, amplitude halving; a corner's gradient is one of eight of length √5,
// picked by the hash of its wrapped lattice index and its octave.
//
// THE DETAIL is one minus the distance to the nearest feature point over the
// cell's size, over three octaves of 4 to 16 cells a side, amplitude halving;
// a cell's one point is placed by the hash of its index and octave, and a
// neighbour past the edge takes the point of the cell it wraps to.
//
// CONSTRAINTS. The nearest point is sought in the 3 × 3 × 3 cells about a
// texel, which misses a nearer one only when none is within one cell size.
// The detail is 64³ texels × 81 cells, about 85 ms at -O2 at device open;
// skipping a neighbour cell no nearer than the nearest point found would cut
// it if a profile ever names it.
#include "clouds_noise.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define WEATHER ((uint32_t)VOE_RENDER_CLOUDS_WEATHER_SIZE)
#define DETAIL ((uint32_t)VOE_RENDER_CLOUDS_DETAIL_SIZE)
#define ONE ((int64_t)65536)
#define WEATHER_CELLS 4u
#define WEATHER_OCTAVES 5u
#define DETAIL_CELLS 4u
#define DETAIL_OCTAVES 3u
#define FINE ((int64_t)256)
#define DETAIL_METRES 256
// Feature points of all detail octaves: 4³ + 8³ + 16³.
#define DETAIL_POINTS (64u + 512u + 4096u)
// Seeds the detail's hashes apart from the weather's.
#define DETAIL_SEED 16u

static_assert((int)VOE_RENDER_CLOUDS_PERIOD % DETAIL_METRES == 0,
	      "the detail repeats a whole number of times over the period");
static_assert(WEATHER % (WEATHER_CELLS << (WEATHER_OCTAVES - 1)) == 0,
	      "every weather lattice period divides the size");
static_assert(DETAIL % (DETAIL_CELLS << (DETAIL_OCTAVES - 1)) == 0,
	      "every detail cell period divides the size");

// A cell's feature point, fine units from the cell's lowest corner.
struct feature_point {
	int32_t x, y, z;
};

static const int64_t GRADIENTS[8][2] = { { 2, 1 },   { 1, 2 },	 { -1, 2 },
					 { -2, 1 },  { -2, -1 }, { -1, -2 },
					 { 1, -2 },  { 2, -1 } };

// Chris Wellons' lowbias32: an integer hash with every input bit reaching
// every output bit.
static uint32_t mix(uint32_t x)
{
	x ^= x >> 16;
	x *= 0x7feb352du;
	x ^= x >> 15;
	x *= 0x846ca68bu;
	x ^= x >> 16;
	return x;
}

static uint32_t hash3(uint32_t x, uint32_t y, uint32_t z, uint32_t seed)
{
	return mix(x + mix(y + mix(z + mix(seed))));
}

static int64_t fade(int64_t t)
{
	assert(t >= 0 && t <= ONE);
	const int64_t t3 = t * t / ONE * t / ONE;
	const int64_t inner = t * (6 * t - 15 * ONE) / ONE + 10 * ONE;
	const int64_t f = t3 * inner / ONE;

	assert(f >= 0 && f <= ONE);
	return f;
}

static int64_t lerp(int64_t a, int64_t b, int64_t f)
{
	assert(f >= 0 && f <= ONE);
	return a + (b - a) * f / ONE;
}

// Lattice corner (cx, cy), wrapped at `cells`, its gradient dotted with the
// Q16 offset (dx, dy) from it.
static int64_t corner(uint32_t cx, uint32_t cy, uint32_t cells,
		      uint32_t octave, int64_t dx, int64_t dy)
{
	assert(cells > 0);
	const uint32_t g = hash3(cx % cells, cy % cells, 0, octave) & 7u;

	assert(g < 8);
	return GRADIENTS[g][0] * dx + GRADIENTS[g][1] * dy;
}

// One octave of gradient noise at texel (x, y)'s centre.
static int64_t perlin(uint32_t x, uint32_t y, uint32_t octave)
{
	const uint32_t cells = WEATHER_CELLS << octave;
	const uint32_t side = WEATHER / cells;
	const uint32_t cx = x / side;
	const uint32_t cy = y / side;
	const int64_t fx = (int64_t)(2 * (x % side) + 1) * ONE / (2 * side);
	const int64_t fy = (int64_t)(2 * (y % side) + 1) * ONE / (2 * side);

	assert(x < WEATHER && y < WEATHER);
	const int64_t u = fade(fx);
	const int64_t low = lerp(corner(cx, cy, cells, octave, fx, fy),
				 corner(cx + 1, cy, cells, octave, fx - ONE, fy),
				 u);
	const int64_t high =
		lerp(corner(cx, cy + 1, cells, octave, fx, fy - ONE),
		     corner(cx + 1, cy + 1, cells, octave, fx - ONE, fy - ONE),
		     u);

	return lerp(low, high, fade(fy));
}

static int64_t weather_at(uint32_t x, uint32_t y)
{
	int64_t sum = 0;

	assert(x < WEATHER && y < WEATHER);
	for (uint32_t o = 0; o < WEATHER_OCTAVES; o++)
		sum += perlin(x, y, o) / ((int64_t)1 << o);
	return sum;
}

// `v` of [lo, hi] as a byte, lo 0 and hi 255, rounded to nearest.
static uint8_t to_byte(int64_t v, int64_t lo, int64_t hi)
{
	assert(lo <= v && v <= hi);
	if (hi == lo)
		return 0;
	const int64_t b = ((v - lo) * 255 + (hi - lo) / 2) / (hi - lo);

	assert(b >= 0 && b <= 255);
	return (uint8_t)b;
}

void voe_render_clouds_weather(uint8_t *texels)
{
	assert(texels != NULL);
	int64_t lo = INT64_MAX;
	int64_t hi = INT64_MIN;

	for (uint32_t y = 0; y < WEATHER; y++)
		for (uint32_t x = 0; x < WEATHER; x++) {
			const int64_t v = weather_at(x, y);

			lo = v < lo ? v : lo;
			hi = v > hi ? v : hi;
		}
	for (uint32_t y = 0; y < WEATHER; y++)
		for (uint32_t x = 0; x < WEATHER; x++)
			texels[x + WEATHER * y] = to_byte(weather_at(x, y), lo, hi);
	assert(lo < hi);
}

// The square root of `n`, rounded down, bit by bit: 32 steps at most.
static int64_t isqrt(uint64_t n)
{
	uint64_t root = 0;
	uint64_t bit = (uint64_t)1 << 62;

	while (bit > n)
		bit >>= 2;
	while (bit != 0) {
		if (n >= root + bit) {
			n -= root + bit;
			root = (root >> 1) + bit;
		} else {
			root >>= 1;
		}
		bit >>= 2;
	}
	assert(root <= UINT32_MAX);
	return (int64_t)root;
}

static uint32_t first_point(uint32_t octave)
{
	uint32_t first = 0;

	assert(octave < DETAIL_OCTAVES);
	for (uint32_t o = 0; o < octave; o++) {
		const uint32_t cells = DETAIL_CELLS << o;

		first += cells * cells * cells;
	}
	assert(first < DETAIL_POINTS);
	return first;
}

// Every detail cell's feature point, octave after octave, x fastest.
static void place_points(struct feature_point points[DETAIL_POINTS])
{
	assert(points != NULL);
	for (uint32_t o = 0; o < DETAIL_OCTAVES; o++) {
		const uint32_t cells = DETAIL_CELLS << o;
		const int64_t size = (int64_t)(DETAIL / cells) * FINE;
		const uint32_t first = first_point(o);

		for (uint32_t i = 0; i < cells * cells * cells; i++) {
			const uint32_t h = hash3(i % cells, i / cells % cells,
						 i / cells / cells,
						 DETAIL_SEED + o);

			points[first + i] = (struct feature_point){
				(int32_t)((h & 1023u) * size / 1024),
				(int32_t)((h >> 10 & 1023u) * size / 1024),
				(int32_t)((h >> 20 & 1023u) * size / 1024)
			};
		}
	}
	assert(first_point(DETAIL_OCTAVES - 1) + 4096u == DETAIL_POINTS);
}

// The three cells about texel `t` on one axis, wrapped at `cells` (a power of
// two) into `index` times `stride`, and each cell's corner less the texel's
// centre, in fine units, into `corner`.
static void neighbours(uint32_t t, uint32_t cells, uint32_t stride,
		       uint32_t index[3], int32_t corner[3])
{
	const uint32_t side = DETAIL / cells;
	const int32_t centre = (int32_t)(t * FINE + FINE / 2);

	assert(t < DETAIL && (cells & (cells - 1)) == 0);
	for (uint32_t i = 0; i < 3; i++) {
		const uint32_t cell = t / side + cells - 1 + i;

		index[i] = (cell & (cells - 1)) * stride;
		corner[i] = ((int32_t)cell - (int32_t)cells) * (int32_t)side *
				    (int32_t)FINE -
			    centre;
	}
	assert(index[1] == (t / side) * stride);
}

// One octave of inverted Worley noise at texel (x, y, z)'s centre: 1 at a
// feature point, 0 a cell's size or more from all.
static int64_t worley(const struct feature_point *points, uint32_t x,
		      uint32_t y, uint32_t z, uint32_t octave)
{
	const uint32_t cells = DETAIL_CELLS << octave;
	const int64_t size = (int64_t)(DETAIL / cells) * FINE;
	const struct feature_point *own = points + first_point(octave);
	uint32_t ix[3], iy[3], iz[3];
	int32_t cx[3], cy[3], cz[3];
	int32_t nearest = INT32_MAX;

	neighbours(x, cells, 1, ix, cx);
	neighbours(y, cells, cells, iy, cy);
	neighbours(z, cells, cells * cells, iz, cz);
	for (uint32_t k = 0; k < 3; k++)
		for (uint32_t j = 0; j < 3; j++)
			for (uint32_t i = 0; i < 3; i++) {
				const struct feature_point q =
					own[ix[i] + iy[j] + iz[k]];
				const int32_t ex = cx[i] + q.x;
				const int32_t ey = cy[j] + q.y;
				const int32_t ez = cz[k] + q.z;
				const int32_t d = ex * ex + ey * ey + ez * ez;

				nearest = d < nearest ? d : nearest;
			}
	assert(nearest >= 0);
	const int64_t distance = isqrt((uint64_t)nearest);

	return distance >= size ? 0 : ONE - distance * ONE / size;
}

static int64_t detail_at(const struct feature_point *points, uint32_t x,
			 uint32_t y, uint32_t z)
{
	int64_t sum = 0;

	assert(x < DETAIL && y < DETAIL && z < DETAIL);
	for (uint32_t o = 0; o < DETAIL_OCTAVES; o++)
		sum += worley(points, x, y, z, o) / ((int64_t)1 << o);
	return sum;
}

void voe_render_clouds_detail(uint8_t *texels)
{
	assert(texels != NULL);
	struct feature_point points[DETAIL_POINTS];
	int64_t most = 0;

	for (uint32_t o = 0; o < DETAIL_OCTAVES; o++)
		most += ONE / ((int64_t)1 << o);
	place_points(points);
	for (uint32_t z = 0; z < DETAIL; z++)
		for (uint32_t y = 0; y < DETAIL; y++)
			for (uint32_t x = 0; x < DETAIL; x++)
				texels[x + DETAIL * (y + DETAIL * z)] = to_byte(
					detail_at(points, x, y, z), 0, most);
	assert(most > ONE);
}
