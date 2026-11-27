// The glb a copy picks by its group's shares, and a group's glbs added,
// replaced and removed (0521 points 3-4). One glb always picks 0; six glbs of
// share 1 under seed 3 pick 2 at (1733, -911), as `voe_3d_tree_pick(3, 1733,
// -911, 6)` did; over 10000 positions four equal shares each come up 25% ± 5%,
// shares 3, 1, 1, 1 the first 50% ± 3%, a share of 0 never and all shares 0
// as all 1; a position picks the same glb twice. Adds are refused at the caps
// and for a path the group holds, a replace with another glb's path is
// refused, and a removal moves the later glbs down with their looks.
//
// No file and no arena: the landscape is a zeroed value holding only groups.
#include <assets/landscape.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

enum { SIDE = 100, POSITIONS = SIDE * SIDE };

// Group 0 of `count` glbs `0.glb`, `1.glb`, ... with `shares`, seeded `seed`.
static voe_assets_landscape group_of(uint32_t count, const uint8_t *shares,
				     uint32_t seed)
{
	voe_assets_landscape landscape = { 0 };
	voe_assets_landscape_group *group = &landscape.groups[0];

	for (uint32_t j = 0; j < count; j++) {
		snprintf(group->glbs[j], sizeof(group->glbs[j]), "%u.glb", j);
		group->looks[j] = voe_assets_landscape_look_default();
		group->looks[j].share = shares[j];
	}
	group->glb_count = count;
	group->seed = (uint8_t)(seed + 1);
	landscape.group_count = 1;
	return landscape;
}

// How often each glb of group 0 comes up over a grid of positions, uneven
// steps so no lattice lines up with the hash.
static void tally(const voe_assets_landscape *landscape,
		  uint32_t counts[VOE_ASSETS_LANDSCAPE_GROUP_GLBS])
{
	memset(counts, 0, VOE_ASSETS_LANDSCAPE_GROUP_GLBS * sizeof(*counts));
	for (int32_t r = 0; r < SIDE; r++)
		for (int32_t c = 0; c < SIDE; c++)
			counts[voe_assets_landscape_glb_pick(
				landscape, 0, c * 1237 - 61000,
				r * 977 - 48000)]++;
}

static bool near_percent(uint32_t count, uint32_t percent, uint32_t spread)
{
	return count * 100 >= (percent - spread) * POSITIONS &&
	       count * 100 <= (percent + spread) * POSITIONS;
}

static void one_glb_picks_zero(void)
{
	const uint8_t shares[1] = { 7 };
	const voe_assets_landscape landscape = group_of(1, shares, 5);
	uint32_t counts[VOE_ASSETS_LANDSCAPE_GROUP_GLBS];

	tally(&landscape, counts);
	VOE_TEST_CHECK_INT(counts[0], POSITIONS);
}

static void equal_shares_pick_as_the_tree_pick_did(void)
{
	const uint8_t shares[6] = { 1, 1, 1, 1, 1, 1 };
	const voe_assets_landscape landscape = group_of(6, shares, 3);

	VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&landscape, 0), 3);
	VOE_TEST_CHECK_INT(
		voe_assets_landscape_glb_pick(&landscape, 0, 1733, -911), 2);
}

static void shares_spread_the_picks(void)
{
	const uint8_t equal[4] = { 1, 1, 1, 1 };
	const uint8_t heavy[4] = { 3, 1, 1, 1 };
	const uint8_t one_zero[3] = { 1, 0, 1 };
	const uint8_t all_zero[2] = { 0, 0 };
	uint32_t counts[VOE_ASSETS_LANDSCAPE_GROUP_GLBS];
	voe_assets_landscape landscape = group_of(4, equal, 0);

	tally(&landscape, counts);
	for (uint32_t j = 0; j < 4; j++)
		VOE_TEST_CHECK(near_percent(counts[j], 25, 5));
	landscape = group_of(4, heavy, 9);
	tally(&landscape, counts);
	VOE_TEST_CHECK(near_percent(counts[0], 50, 3));
	landscape = group_of(3, one_zero, 2);
	tally(&landscape, counts);
	VOE_TEST_CHECK_INT(counts[1], 0);
	VOE_TEST_CHECK_INT(counts[0] + counts[2], POSITIONS);
	landscape = group_of(2, all_zero, 4);
	tally(&landscape, counts);
	VOE_TEST_CHECK(near_percent(counts[0], 50, 5));
	VOE_TEST_CHECK(near_percent(counts[1], 50, 5));
}

static void a_position_picks_the_same_glb_twice(void)
{
	const uint8_t shares[5] = { 2, 1, 4, 1, 3 };
	const voe_assets_landscape landscape = group_of(5, shares, 11);

	for (int32_t i = 0; i < 1000; i++) {
		const int32_t x = i * 7919 - 4000000, z = 3000000 - i * 6271;

		VOE_TEST_CHECK_INT(
			voe_assets_landscape_glb_pick(&landscape, 0, x, z),
			voe_assets_landscape_glb_pick(&landscape, 0, x, z));
	}
}

static void adds_are_refused_at_the_caps_and_for_a_repeat(void)
{
	voe_assets_landscape landscape = { 0 };
	char path[16];

	for (uint32_t g = 0; g < VOE_ASSETS_LANDSCAPE_GROUPS; g++) {
		snprintf(path, sizeof(path), "g%u.glb", g);
		VOE_TEST_CHECK(voe_assets_landscape_group_add(&landscape, path));
		VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&landscape, g),
				   g);
	}
	VOE_TEST_CHECK_INT(landscape.group_count, VOE_ASSETS_LANDSCAPE_GROUPS);
	VOE_TEST_CHECK(!voe_assets_landscape_group_add(&landscape, "x.glb"));
	VOE_TEST_CHECK_INT(landscape.group_count, VOE_ASSETS_LANDSCAPE_GROUPS);
	VOE_TEST_CHECK(strcmp(landscape.groups[3].glbs[0], "g3.glb") == 0);
	VOE_TEST_CHECK_INT(landscape.groups[3].glb_count, 1);
	VOE_TEST_CHECK_INT(landscape.groups[3].looks[0].share,
			   VOE_ASSETS_LANDSCAPE_SHARE_DEFAULT);

	VOE_TEST_CHECK(!voe_assets_landscape_glb_add(&landscape, 3, "g3.glb"));
	// The same glb in another group is no repeat.
	VOE_TEST_CHECK(voe_assets_landscape_glb_add(&landscape, 3, "g4.glb"));
	for (uint32_t j = 2; j < VOE_ASSETS_LANDSCAPE_GROUP_GLBS; j++) {
		snprintf(path, sizeof(path), "j%u.glb", j);
		VOE_TEST_CHECK(voe_assets_landscape_glb_add(&landscape, 3, path));
	}
	VOE_TEST_CHECK(!voe_assets_landscape_glb_add(&landscape, 3, "new.glb"));
	VOE_TEST_CHECK_INT(landscape.groups[3].glb_count,
			   VOE_ASSETS_LANDSCAPE_GROUP_GLBS);
}

static void a_new_group_after_a_removal_takes_a_free_seed(void)
{
	voe_assets_landscape landscape = { 0 };

	VOE_TEST_CHECK(voe_assets_landscape_group_add(&landscape, "a.glb"));
	VOE_TEST_CHECK(voe_assets_landscape_group_add(&landscape, "b.glb"));
	voe_assets_landscape_group_remove(&landscape, 0);
	VOE_TEST_CHECK(voe_assets_landscape_group_add(&landscape, "c.glb"));
	VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&landscape, 0), 1);
	VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&landscape, 1), 0);
}

static void a_replace_keeps_the_look_or_is_refused(void)
{
	const uint8_t shares[3] = { 1, 4, 2 };
	voe_assets_landscape landscape = group_of(3, shares, 0);
	voe_assets_landscape_group *group = &landscape.groups[0];

	VOE_TEST_CHECK(!voe_assets_landscape_glb_set(&landscape, 0, 1, "2.glb"));
	VOE_TEST_CHECK(strcmp(group->glbs[1], "1.glb") == 0);
	VOE_TEST_CHECK(voe_assets_landscape_glb_set(&landscape, 0, 1, "1.glb"));
	VOE_TEST_CHECK(voe_assets_landscape_glb_set(&landscape, 0, 1, "n.glb"));
	VOE_TEST_CHECK(strcmp(group->glbs[1], "n.glb") == 0);
	VOE_TEST_CHECK_INT(group->looks[1].share, 4);
}

static void a_removal_moves_the_later_glbs_down(void)
{
	const uint8_t shares[4] = { 1, 2, 3, 4 };
	voe_assets_landscape landscape = group_of(4, shares, 0);
	const voe_assets_landscape_group *group = &landscape.groups[0];

	voe_assets_landscape_glb_remove(&landscape, 0, 1);
	VOE_TEST_CHECK_INT(group->glb_count, 3);
	VOE_TEST_CHECK(strcmp(group->glbs[0], "0.glb") == 0);
	VOE_TEST_CHECK(strcmp(group->glbs[1], "2.glb") == 0);
	VOE_TEST_CHECK(strcmp(group->glbs[2], "3.glb") == 0);
	VOE_TEST_CHECK_INT(group->looks[1].share, 3);
	VOE_TEST_CHECK_INT(group->looks[2].share, 4);
	VOE_TEST_CHECK(group->glbs[3][0] == '\0');
	VOE_TEST_CHECK_INT(group->looks[3].share, 0);
	// The removed path is free to add again.
	VOE_TEST_CHECK(voe_assets_landscape_glb_add(&landscape, 0, "1.glb"));
}

int main(void)
{
	one_glb_picks_zero();
	equal_shares_pick_as_the_tree_pick_did();
	shares_spread_the_picks();
	a_position_picks_the_same_glb_twice();
	adds_are_refused_at_the_caps_and_for_a_repeat();
	a_new_group_after_a_removal_takes_a_free_seed();
	a_replace_keeps_the_look_or_is_refused();
	a_removal_moves_the_later_glbs_down();
	return voe_test_result();
}
