// Which tiles and slices a pass's point lights mark (ADR-0320 point 5), with no
// graphics card: the binning is plain arithmetic over one view.
//
// The view is the eye at the origin looking down −Z through a reverse-Z
// perspective of 60°, built as the other render tests build theirs.
//
// A LIGHT 10 m AHEAD MARKS THE MIDDLE TILES AND ITS SLICES, not a corner tile
// and not slice 0. ONE BEHIND THE EYE MARKS NOTHING. ONE AROUND THE EYE MARKS
// EVERY TILE. ONE FAR TO THE SIDE MARKS NO TILE.
//
// LIGHT 40 IS WORD 1 BIT 8, in its tile's mask and in its slice's.
//
// A SLICE IS 0 AT NEAR, THE LAST AT FAR AND PAST IT, AND RISES WITH DISTANCE.
#include "../src/light_bins.h"

#include <testing/test.h>

#include <math.h>
#include <string.h>

#define TILES (VOE_RENDER_LIGHT_TILES_X * VOE_RENDER_LIGHT_TILES_Y)
#define LAST_SLICE (VOE_RENDER_LIGHT_SLICES - 1)
#define NEAR_PLANE 0.1f
#define FAR_PLANE 1000.0f

static struct voe_render_light_bins bins;

static voe_render_view perspective(void)
{
	voe_render_view view = { .view = voe_math_float4x4_identity() };
	const float focal = 1.0f / tanf(1.04719755f * 0.5f);
	const float span = FAR_PLANE - NEAR_PLANE;

	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

static void fill_one(voe_math_float3 position, float range)
{
	const voe_render_view view = perspective();
	const voe_render_point_light light = { .position = position,
					       .range = range,
					       .colour = { 1, 1, 1 } };

	voe_render_light_bins_fill(&view, &light, 1, &bins);
}

static uint32_t tiles_marked(void)
{
	uint32_t n = 0;

	for (uint32_t t = 0; t < TILES; t++)
		n += bins.tiles[t][0] & 1u;
	return n;
}

static uint32_t slices_marked(void)
{
	uint32_t n = 0;

	for (uint32_t s = 0; s < VOE_RENDER_LIGHT_SLICES; s++)
		n += bins.slices[s][0] & 1u;
	return n;
}

static void a_light_ahead_marks_the_middle(void)
{
	fill_one((voe_math_float3){ 0.0f, 0.0f, -10.0f }, 1.0f);
	VOE_TEST_CHECK_INT(bins.tiles[7 + 16 * 4][0], 1u);
	VOE_TEST_CHECK_INT(bins.tiles[8 + 16 * 4][0], 1u);
	VOE_TEST_CHECK_INT(bins.tiles[0][0], 0u);
	VOE_TEST_CHECK_INT(bins.tiles[TILES - 1][0], 0u);
	for (uint32_t s = voe_render_light_slice(9.0f);
	     s <= voe_render_light_slice(11.0f); s++)
		VOE_TEST_CHECK_INT(bins.slices[s][0], 1u);
	VOE_TEST_CHECK_INT(bins.slices[0][0], 0u);
	VOE_TEST_CHECK_INT(bins.slices[voe_render_light_slice(5.0f)][0], 0u);
}

static void a_light_behind_marks_nothing(void)
{
	fill_one((voe_math_float3){ 0.0f, 0.0f, 10.0f }, 1.0f);
	VOE_TEST_CHECK_INT(tiles_marked(), 0);
	VOE_TEST_CHECK_INT(slices_marked(), 0);
}

static void a_light_around_the_eye_marks_every_tile(void)
{
	fill_one((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 2.0f);
	VOE_TEST_CHECK_INT(tiles_marked(), TILES);
	VOE_TEST_CHECK_INT(bins.slices[0][0], 1u);
}

static void a_light_to_the_side_marks_no_tile(void)
{
	fill_one((voe_math_float3){ 100.0f, 0.0f, -10.0f }, 1.0f);
	VOE_TEST_CHECK_INT(tiles_marked(), 0);
}

static void light_40_is_word_1_bit_8(void)
{
	const voe_render_view view = perspective();
	voe_render_point_light lights[41];
	const uint32_t slice = voe_render_light_slice(10.0f);

	for (uint32_t i = 0; i < 41; i++)
		lights[i] = (voe_render_point_light){ .position = { 0, 0, 10 },
						      .range = 1.0f };
	lights[40].position = (voe_math_float3){ 0.0f, 0.0f, -10.0f };
	voe_render_light_bins_fill(&view, lights, 41, &bins);
	VOE_TEST_CHECK_INT(bins.tiles[7 + 16 * 4][0], 0u);
	VOE_TEST_CHECK_INT(bins.tiles[7 + 16 * 4][1], 1u << 8);
	VOE_TEST_CHECK_INT(bins.slices[slice][0], 0u);
	VOE_TEST_CHECK_INT(bins.slices[slice][1], 1u << 8);
}

static void slices_run_near_to_far(void)
{
	uint32_t previous = 0;
	bool rising = true;

	VOE_TEST_CHECK_INT(voe_render_light_slice(VOE_RENDER_LIGHT_SLICE_NEAR), 0);
	VOE_TEST_CHECK_INT(voe_render_light_slice(0.0f), 0);
	VOE_TEST_CHECK_INT(voe_render_light_slice(VOE_RENDER_LIGHT_SLICE_FAR),
			   LAST_SLICE);
	VOE_TEST_CHECK_INT(voe_render_light_slice(1.0e6f), LAST_SLICE);
	for (float d = 0.2f; d < 1000.0f; d *= 1.5f) {
		const uint32_t s = voe_render_light_slice(d);

		rising = rising && s >= previous;
		previous = s;
	}
	VOE_TEST_CHECK(rising);
	VOE_TEST_CHECK(voe_render_light_slice(1.0f) <
		       voe_render_light_slice(100.0f));
}

int main(void)
{
	a_light_ahead_marks_the_middle();
	a_light_behind_marks_nothing();
	a_light_around_the_eye_marks_every_tile();
	a_light_to_the_side_marks_no_tile();
	light_40_is_word_1_bit_8();
	slices_run_near_to_far();
	return voe_test_result();
}
