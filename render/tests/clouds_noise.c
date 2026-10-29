// The cloud field's two baked textures (ADR-0492 point 4), with no graphics
// card: the noise is plain arithmetic into bytes.
//
// EACH FIELD MADE TWICE IS THE SAME BYTES.
//
// EACH TILES: on every axis the largest step across the wrap, from the last
// texel back to the first, is no larger than the largest step between
// neighbours inside.
//
// EACH USES THE BYTE: its darkest texel under 64, its brightest over 192.
#include "../src/clouds_noise.h"

#include <testing/test.h>

#include <stdint.h>
#include <string.h>

#define WEATHER ((uint32_t)VOE_RENDER_CLOUDS_WEATHER_SIZE)
#define DETAIL ((uint32_t)VOE_RENDER_CLOUDS_DETAIL_SIZE)
#define WEATHER_BYTES (WEATHER * WEATHER)
#define DETAIL_BYTES (DETAIL * DETAIL * DETAIL)

static uint8_t first[DETAIL_BYTES];
static uint8_t second[DETAIL_BYTES];

static uint32_t step(uint8_t a, uint8_t b)
{
	return a > b ? (uint32_t)(a - b) : (uint32_t)(b - a);
}

// Over a field of `side` texels on each of `axes` axes, x fastest, whether
// every axis's largest step across its wrap is within its largest inside.
static bool tiles(const uint8_t *texels, uint32_t side, uint32_t axes)
{
	const uint32_t count = axes == 2 ? side * side : side * side * side;
	bool tiled = true;

	for (uint32_t axis = 0, stride = 1; axis < axes; axis++, stride *= side) {
		uint32_t inside = 0;
		uint32_t across = 0;

		for (uint32_t i = 0; i < count; i++) {
			const uint32_t at = i / stride % side;
			const uint32_t next = at + 1 == side ? i - at * stride :
							       i + stride;
			const uint32_t s = step(texels[i], texels[next]);

			if (at + 1 == side)
				across = s > across ? s : across;
			else
				inside = s > inside ? s : inside;
		}
		tiled = tiled && across <= inside;
	}
	return tiled;
}

static void check_range(const uint8_t *texels, uint32_t count)
{
	uint8_t darkest = 255;
	uint8_t brightest = 0;

	for (uint32_t i = 0; i < count; i++) {
		darkest = texels[i] < darkest ? texels[i] : darkest;
		brightest = texels[i] > brightest ? texels[i] : brightest;
	}
	VOE_TEST_CHECK(darkest < 64);
	VOE_TEST_CHECK(brightest > 192);
}

static void the_weather(void)
{
	voe_render_clouds_weather(first);
	voe_render_clouds_weather(second);
	VOE_TEST_CHECK(memcmp(first, second, WEATHER_BYTES) == 0);
	VOE_TEST_CHECK(tiles(first, WEATHER, 2));
	check_range(first, WEATHER_BYTES);
}

static void the_detail(void)
{
	voe_render_clouds_detail(first);
	voe_render_clouds_detail(second);
	VOE_TEST_CHECK(memcmp(first, second, DETAIL_BYTES) == 0);
	VOE_TEST_CHECK(tiles(first, DETAIL, 3));
	check_range(first, DETAIL_BYTES);
}

int main(void)
{
	the_weather();
	the_detail();
	return voe_test_result();
}
