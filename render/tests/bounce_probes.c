// Which captured bounce probes are queued and when the grid relights
// (ADR-0326 points 2, 4 and 6), with no graphics card: plain bookkeeping over
// a 24 × 12 × 24 toroidal grid centred on the eye.
//
// THE FIRST PLACE QUEUES ALL 6912 and marks them changed.
//
// A TAKE OF 16 GIVES THE 16 NEAREST THE EYE: none left queued is nearer than
// any taken, and the taken hold a picture.
//
// A MOVE OF ONE CELL ALONG +x QUEUES 288, the slab of the new highest column,
// and every other probe keeps its picture.
//
// CELL −1 WRAPS TO 23, and the index is x + 24·y + 288·z of the wrapped cell.
//
// A STALE SPHERE OF 6 m QUEUES ONLY THE PROBES WHOSE CENTRE IS WITHIN IT.
//
// NOTHING CHANGED AND THE SAME LIGHTS NEED NO RELIGHT; a lamp's bounce
// strength changed does.
//
// THE 17th BOUNCING LAMP IS LEFT OUT, and lamps with no bounces are skipped.
//
// The tests set the marks by hand where taking all 6912 one by one would only
// be slow: a grid with every probe captured is the state those claims start in.
#include "../src/bounce_probes.h"

#include <testing/test.h>

#include <string.h>

#define TOTAL VOE_RENDER_BOUNCE_PROBES_TOTAL
#define XZ VOE_RENDER_BOUNCE_PROBES_XZ
#define H VOE_RENDER_BOUNCE_PROBES_Y

static const voe_render_light SUN = {
	.direction = { 0.0f, -1.0f, 0.0f },
	.intensity = 1.0f,
	.colour = { 1.0f, 1.0f, 1.0f },
};

static const int32_t CELL[3] = { -12, -6, -12 };
static const voe_math_float3 CORNER = { -24.0f, -12.0f, -24.0f };
static const voe_render_point_lights NONE = { 0 };

static voe_render_bounce_lights lights;

static void place(voe_render_bounce_probes *p, const int32_t cell[3],
		  const voe_math_float4 *stale, uint32_t stale_count,
		  const voe_render_point_lights *lamps)
{
	voe_render_bounce_probes_place(p, cell, CORNER, stale, stale_count, &SUN,
				       1, 1.0f, lamps, &lights);
}

static uint32_t count(const uint32_t *bits)
{
	uint32_t n = 0;

	for (uint32_t i = 0; i < TOTAL; i++)
		n += (bits[i / 32] >> (i & 31u)) & 1u;
	return n;
}

// Every probe captured and nothing queued, as after taking them all.
static void capture_all(voe_render_bounce_probes *p)
{
	memset(p->holds, 0xff, sizeof(p->holds));
	memset(p->queued, 0, sizeof(p->queued));
}

static bool has(const uint32_t *bits, uint32_t probe)
{
	return (bits[probe / 32] >> (probe & 31u)) & 1u;
}

static void the_first_place_queues_all(void)
{
	static voe_render_bounce_probes p;

	memset(&p, 0, sizeof(p));
	place(&p, CELL, NULL, 0, &NONE);
	VOE_TEST_CHECK_INT(count(p.queued), TOTAL);
	VOE_TEST_CHECK_INT(count(p.changed), TOTAL);
	VOE_TEST_CHECK_INT(count(p.holds), 0);
}

static void a_take_gives_the_nearest(void)
{
	static voe_render_bounce_probes p;
	static float dist[TOTAL];
	uint32_t taken[16];
	float far_taken = 0.0f, near_left = 1e30f;

	memset(&p, 0, sizeof(p));
	place(&p, CELL, NULL, 0, &NONE);
	for (uint32_t z = 0; z < XZ; z++)
		for (uint32_t y = 0; y < H; y++)
			for (uint32_t x = 0; x < XZ; x++) {
				const float cx = CORNER.x + (x + 0.5f) * 2.0f;
				const float cy = CORNER.y + (y + 0.5f) * 2.0f;
				const float cz = CORNER.z + (z + 0.5f) * 2.0f;

				dist[voe_render_bounce_probe_index(
					(int64_t)CELL[0] + x, (int64_t)CELL[1] + y, (int64_t)CELL[2] + z)] =
					cx * cx + cy * cy + cz * cz;
			}
	VOE_TEST_CHECK_INT(voe_render_bounce_probes_take(&p, taken, 16), 16);
	for (uint32_t i = 0; i < 16; i++) {
		VOE_TEST_CHECK(has(p.holds, taken[i]) && !has(p.queued, taken[i]));
		far_taken = dist[taken[i]] > far_taken ? dist[taken[i]] : far_taken;
	}
	for (uint32_t i = 0; i < TOTAL; i++)
		if (has(p.queued, i) && dist[i] < near_left)
			near_left = dist[i];
	VOE_TEST_CHECK_INT(count(p.queued), TOTAL - 16);
	VOE_TEST_CHECK(far_taken <= near_left);
}

static void a_move_queues_the_entered_slab(void)
{
	static voe_render_bounce_probes p;
	const int32_t moved[3] = { CELL[0] + 1, CELL[1], CELL[2] };
	const uint32_t column = voe_render_bounce_probe_wrap(moved[0] + XZ - 1, XZ);
	bool in_column = true;

	memset(&p, 0, sizeof(p));
	place(&p, CELL, NULL, 0, &NONE);
	capture_all(&p);
	voe_render_bounce_probes_relit(&p, &lights);
	place(&p, moved, NULL, 0, &NONE);
	VOE_TEST_CHECK_INT(count(p.queued), 288);
	VOE_TEST_CHECK_INT(count(p.changed), 288);
	VOE_TEST_CHECK_INT(count(p.holds), TOTAL - 288);
	for (uint32_t i = 0; i < TOTAL; i++)
		if (has(p.queued, i))
			in_column = in_column && i % XZ == column;
	VOE_TEST_CHECK(in_column);
}

static void minus_one_wraps(void)
{
	VOE_TEST_CHECK_INT(voe_render_bounce_probe_wrap(-1, XZ), 23);
	VOE_TEST_CHECK_INT(voe_render_bounce_probe_wrap(-1, H), 11);
	VOE_TEST_CHECK_INT(voe_render_bounce_probe_index(-1, -1, -1),
			   23 + 24 * 11 + 288 * 23);
	VOE_TEST_CHECK_INT(voe_render_bounce_probe_index(-25, 12, 24), 23);
}

static void a_stale_sphere_queues_within(void)
{
	static voe_render_bounce_probes p;
	const voe_math_float4 sphere = { 3.0f, -1.0f, 5.0f, 6.0f };
	uint32_t within = 0;
	bool only_within = true;

	memset(&p, 0, sizeof(p));
	place(&p, CELL, NULL, 0, &NONE);
	capture_all(&p);
	place(&p, CELL, &sphere, 1, &NONE);
	for (uint32_t z = 0; z < XZ; z++)
		for (uint32_t y = 0; y < H; y++)
			for (uint32_t x = 0; x < XZ; x++) {
				const float dx = CORNER.x + (x + 0.5f) * 2.0f - sphere.x;
				const float dy = CORNER.y + (y + 0.5f) * 2.0f - sphere.y;
				const float dz = CORNER.z + (z + 0.5f) * 2.0f - sphere.z;
				const bool in = dx * dx + dy * dy + dz * dz <= 36.0f;
				const uint32_t probe = voe_render_bounce_probe_index(
					(int64_t)CELL[0] + x, (int64_t)CELL[1] + y, (int64_t)CELL[2] + z);

				within += in;
				only_within = only_within && in == has(p.queued, probe);
			}
	VOE_TEST_CHECK(within > 0);
	VOE_TEST_CHECK_INT(count(p.queued), within);
	VOE_TEST_CHECK(only_within);
}

static void relight_follows_changes_and_lights(void)
{
	static voe_render_bounce_probes p;
	voe_render_point_light lamp = {
		.range = 5.0f, .colour = { 1, 1, 1 }, .falloff = 1.0f,
		.bounces = 1, .bounce_strength = 1.0f,
	};
	const voe_render_point_lights lamps = { &lamp, 1 };

	memset(&p, 0, sizeof(p));
	place(&p, CELL, NULL, 0, &lamps);
	VOE_TEST_CHECK(voe_render_bounce_probes_relight_needed(&p, &lights));
	voe_render_bounce_probes_relit(&p, &lights);
	place(&p, CELL, NULL, 0, &lamps);
	VOE_TEST_CHECK(!voe_render_bounce_probes_relight_needed(&p, &lights));
	lamp.bounce_strength = 2.0f;
	place(&p, CELL, NULL, 0, &lamps);
	VOE_TEST_CHECK(voe_render_bounce_probes_relight_needed(&p, &lights));
}

static void the_17th_bouncing_lamp_is_left_out(void)
{
	static voe_render_bounce_probes p;
	voe_render_point_light all[20];
	const voe_render_point_lights lamps = { all, 20 };

	memset(all, 0, sizeof(all));
	for (uint32_t i = 0; i < 20; i++) {
		all[i].range = (float)i;
		all[i].falloff = 1.0f;
		all[i].bounces = i == 2 ? 0 : 1;
	}
	memset(&p, 0, sizeof(p));
	place(&p, CELL, NULL, 0, &lamps);
	VOE_TEST_CHECK_INT(lights.lamp_count, 16);
	VOE_TEST_CHECK(lights.lamps[1].range == 1.0f);
	VOE_TEST_CHECK(lights.lamps[2].range == 3.0f);
	VOE_TEST_CHECK(lights.lamps[15].range == 16.0f);
}

int main(void)
{
	the_first_place_queues_all();
	a_take_gives_the_nearest();
	a_move_queues_the_entered_slab();
	minus_one_wraps();
	a_stale_sphere_queues_within();
	relight_follows_changes_and_lights();
	the_17th_bouncing_lamp_is_left_out();
	return voe_test_result();
}
