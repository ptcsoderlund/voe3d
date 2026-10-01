// Which bounce probes an update refreshes (ADR-0308 point 5), with no graphics
// card: the schedule is plain arithmetic over a 32³ toroidal grid.
//
// THE FIRST CALL IS THE WHOLE GRID: all 32768 indices, blend 1, each once.
//
// NOTHING CHANGED IS THE CYCLE: 4096 indices, none twice, and eight such calls
// visit every cell.
//
// A MOVE OF ONE CELL IN +x LISTS THE 1024 CELLS IT BRINGS IN FIRST, blend 1 —
// the slab whose x mod 32 is the new highest column.
//
// A 3 m STALE SPHERE AT A CELL'S CENTRE LISTS THAT CELL AMONG THE 19 WHOSE
// CENTRES ARE WITHIN 3 m, before any cycle cell, at the update's blend.
//
// A CHANGED LIGHT COLOUR RESTARTS THE CYCLE: three calls in, eight more after
// the change still visit every cell.
//
// A ROOM SMALLER THAN THE ENTERED CELLS TRUNCATES: the count is the room and
// nothing past it is written.
#include "../src/bounce_schedule.h"

#include <testing/test.h>

#include <string.h>

#define ALL VOE_RENDER_BOUNCE_PROBES_ALL
#define SIDE VOE_RENDER_BOUNCE_PROBES

static uint32_t probes[ALL + 1];
static float blends[ALL + 1];
static uint8_t seen[ALL];

static const voe_render_light SUN = {
	.direction = { 0.0f, -1.0f, 0.0f },
	.intensity = 1.0f,
	.colour = { 1.0f, 1.0f, 1.0f },
};

static const voe_math_float3 CORNER = { -32.0f, -32.0f, 0.0f };

static uint32_t next(voe_render_bounce_schedule *s, const int32_t cell[3],
		     const voe_render_light *sun, const voe_math_float4 *stale,
		     uint32_t stale_count)
{
	return voe_render_bounce_schedule_next(s, cell, CORNER, sun, stale,
					       stale_count, probes, blends, ALL);
}

// Adds a call's list to `seen`; false if any index is out of range or twice.
static bool note(uint32_t n)
{
	static uint8_t this_call[ALL];
	bool ok = true;

	memset(this_call, 0, sizeof(this_call));
	for (uint32_t i = 0; i < n; i++) {
		if (probes[i] >= ALL || this_call[probes[i]]) {
			ok = false;
			continue;
		}
		this_call[probes[i]] = 1;
		seen[probes[i]] = 1;
	}
	return ok;
}

static uint32_t count_seen(void)
{
	uint32_t n = 0;

	for (uint32_t i = 0; i < ALL; i++)
		n += seen[i];
	return n;
}

static void first_call_is_the_whole_grid(void)
{
	voe_render_bounce_schedule s = { 0 };
	const int32_t cell[3] = { -16, 3, 0 };
	const uint32_t n = next(&s, cell, &SUN, NULL, 0);
	bool all_one = true;

	memset(seen, 0, sizeof(seen));
	VOE_TEST_CHECK_INT(n, ALL);
	VOE_TEST_CHECK(note(n));
	VOE_TEST_CHECK_INT(count_seen(), ALL);
	for (uint32_t i = 0; i < n; i++)
		all_one = all_one && blends[i] == 1.0f;
	VOE_TEST_CHECK(all_one);
}

static void nothing_changed_cycles(void)
{
	voe_render_bounce_schedule s = { 0 };
	const int32_t cell[3] = { 5, -7, 2 };

	next(&s, cell, &SUN, NULL, 0);
	memset(seen, 0, sizeof(seen));
	for (int call = 0; call < 8; call++) {
		const uint32_t n = next(&s, cell, &SUN, NULL, 0);

		VOE_TEST_CHECK_INT(n, VOE_RENDER_BOUNCE_BUDGET);
		VOE_TEST_CHECK(note(n));
		VOE_TEST_CHECK_FLOAT(blends[0], VOE_RENDER_BOUNCE_BLEND, 0.0);
	}
	VOE_TEST_CHECK_INT(count_seen(), ALL);
}

static void a_move_lists_the_entered_cells_first(void)
{
	voe_render_bounce_schedule s = { 0 };
	const int32_t cell[3] = { -1, 0, 4 };
	const int32_t moved[3] = { 0, 0, 4 };
	const uint32_t column = (uint32_t)(moved[0] + SIDE - 1) % SIDE;
	bool entered = true;

	next(&s, cell, &SUN, NULL, 0);
	const uint32_t n = next(&s, moved, &SUN, NULL, 0);

	VOE_TEST_CHECK_INT(n, 1024 + VOE_RENDER_BOUNCE_BUDGET);
	VOE_TEST_CHECK(note(n));
	for (uint32_t i = 0; i < 1024; i++)
		entered = entered && probes[i] % SIDE == column &&
			  blends[i] == 1.0f;
	VOE_TEST_CHECK(entered);
	VOE_TEST_CHECK_FLOAT(blends[1024], VOE_RENDER_BOUNCE_BLEND, 0.0);
}

static void a_stale_sphere_comes_before_the_cycle(void)
{
	voe_render_bounce_schedule s = { 0 };
	const int32_t cell[3] = { 10, 20, -30 };
	const uint32_t x = 5, y = 6, z = 7;
	const uint32_t probe = (10 + x) % SIDE + SIDE * ((20 + y) % SIDE) +
			       SIDE * SIDE * ((uint32_t)(-30 + 32 + (int)z) % SIDE);
	const voe_math_float4 sphere = {
		CORNER.x + (x + 0.5f) * VOE_RENDER_BOUNCE_SPACING,
		CORNER.y + (y + 0.5f) * VOE_RENDER_BOUNCE_SPACING,
		CORNER.z + (z + 0.5f) * VOE_RENDER_BOUNCE_SPACING, 3.0f,
	};
	int32_t at = -1;

	next(&s, cell, &SUN, NULL, 0);
	const uint32_t n = next(&s, cell, &SUN, &sphere, 1);

	VOE_TEST_CHECK_INT(n, VOE_RENDER_BOUNCE_BUDGET);
	VOE_TEST_CHECK(note(n));
	for (uint32_t i = 0; i < n && at < 0; i++)
		if (probes[i] == probe)
			at = (int32_t)i;
	VOE_TEST_CHECK(at >= 0 && at < 19);
	if (at >= 0)
		VOE_TEST_CHECK_FLOAT(blends[at], VOE_RENDER_BOUNCE_BLEND, 0.0);
}

static void a_changed_light_restarts_the_cycle(void)
{
	voe_render_bounce_schedule s = { 0 };
	const int32_t cell[3] = { 0, 0, 0 };
	voe_render_light red = SUN;
	static uint32_t restart[VOE_RENDER_BOUNCE_BUDGET];

	// The cycle's opening list, from a fresh schedule.
	next(&s, cell, &SUN, NULL, 0);
	next(&s, cell, &SUN, NULL, 0);
	memcpy(restart, probes, sizeof(restart));

	red.colour = (voe_math_float3){ 1.0f, 0.2f, 0.2f };
	for (int call = 0; call < 3; call++)
		next(&s, cell, &SUN, NULL, 0);
	memset(seen, 0, sizeof(seen));
	for (int call = 0; call < 8; call++) {
		const uint32_t n = next(&s, cell, &red, NULL, 0);

		VOE_TEST_CHECK_INT(n, VOE_RENDER_BOUNCE_BUDGET);
		VOE_TEST_CHECK(note(n));
		if (call == 0)
			VOE_TEST_CHECK(memcmp(probes, restart,
					      sizeof(restart)) == 0);
	}
	VOE_TEST_CHECK_INT(count_seen(), ALL);
}

static void a_small_room_truncates(void)
{
	voe_render_bounce_schedule s = { 0 };
	const int32_t cell[3] = { 0, 0, 0 };
	const int32_t moved[3] = { 1, 0, 0 };
	const uint32_t room = 100;

	next(&s, cell, &SUN, NULL, 0);
	probes[room] = 0xdeadbeefu;
	blends[room] = -1.0f;
	const uint32_t n = voe_render_bounce_schedule_next(
		&s, moved, CORNER, &SUN, NULL, 0, probes, blends, room);

	VOE_TEST_CHECK_INT(n, room);
	VOE_TEST_CHECK_INT(probes[room], 0xdeadbeefu);
	VOE_TEST_CHECK(blends[room] == -1.0f);
}

int main(void)
{
	first_call_is_the_whole_grid();
	nothing_changed_cycles();
	a_move_lists_the_entered_cells_first();
	a_stale_sphere_comes_before_the_cycle();
	a_changed_light_restarts_the_cycle();
	a_small_room_truncates();
	return voe_test_result();
}
