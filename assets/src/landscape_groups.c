// A landscape's foliage groups as a set: a glb's base size, each group's seed,
// the seed a group added takes, and a group removed (0516, 0517 point 4, 0520).
// Called by whatever keys a draw by a group (the grown lattice's hash, the glb
// pick), sizes a copy by its glb, and by the editor adding or removing a group.
//
// A REMOVAL DROPS THE GROUP'S GLBS, LOOKS, SEED, EVERY LAYER'S GROWTH OF IT AND
// ITS COPIES; later groups move down one with their own, their copies keep
// their order with the group lowered, and the freed last slot is zeroed. LATER
// GROUPS KEEP THEIR SEEDS: each is made explicit before anything moves, so
// their draws stay where they were. Nothing allocates: the copies close up in
// place.
//
// A GROUP'S SEED IS ITS INDEX UNTIL A GROUP IS REMOVED: its seed is the seed
// plus one and 0 means its index, so a file that never named a seed draws as
// it always did, and a group moved down by a removal keeps the seed it had.
//
// A NEW GROUP'S SEED IS ITS OWN INDEX WHEN FREE, else the least seed no group
// has: there are VOE_ASSETS_LANDSCAPE_GROUPS seeds and fewer groups before the
// new one, so one is always free. Both are scans of at most 16 groups.
#include <assets/landscape.h>
#include <base/assert.h>

#include <string.h>

float voe_assets_landscape_glb_size(const voe_assets_landscape *landscape,
				    uint32_t group, uint32_t glb)
{
	VOE_BASE_ASSERT(landscape != NULL, "the glb size of no landscape");
	VOE_BASE_ASSERT(group < VOE_ASSETS_LANDSCAPE_GROUPS &&
				glb < VOE_ASSETS_LANDSCAPE_GROUP_GLBS,
			"a glb past the most a landscape holds");

	const uint16_t thousandths = landscape->groups[group].looks[glb].size;

	return thousandths == 0 ? 1.0f : (float)thousandths / 1000.0f;
}

uint32_t voe_assets_landscape_group_seed(const voe_assets_landscape *landscape,
					 uint32_t group)
{
	VOE_BASE_ASSERT(landscape != NULL, "the group seed of no landscape");
	VOE_BASE_ASSERT(group < VOE_ASSETS_LANDSCAPE_GROUPS,
			"a group past the most a landscape holds");

	const uint8_t seed = landscape->groups[group].seed;

	VOE_BASE_ASSERT(seed <= VOE_ASSETS_LANDSCAPE_GROUPS,
			"a seed past the most a landscape holds");
	return seed == 0 ? group : (uint32_t)seed - 1;
}

uint32_t voe_assets_landscape_seed_new(const voe_assets_landscape *landscape)
{
	VOE_BASE_ASSERT(landscape != NULL, "a new seed in no landscape");
	VOE_BASE_ASSERT(landscape->group_count < VOE_ASSETS_LANDSCAPE_GROUPS,
			"a new seed in a landscape with every group");

	bool taken[VOE_ASSETS_LANDSCAPE_GROUPS] = { false };
	uint32_t seed = 0;

	for (uint32_t g = 0; g < landscape->group_count; g++)
		taken[voe_assets_landscape_group_seed(landscape, g)] = true;
	if (!taken[landscape->group_count])
		return landscape->group_count;
	while (seed < VOE_ASSETS_LANDSCAPE_GROUPS && taken[seed])
		seed++;
	VOE_BASE_ASSERT(seed < VOE_ASSETS_LANDSCAPE_GROUPS,
			"no seed free with a group's room free");
	return seed;
}

void voe_assets_landscape_group_remove(voe_assets_landscape *landscape,
				       uint32_t group)
{
	VOE_BASE_ASSERT(landscape != NULL, "a group removed from no landscape");
	VOE_BASE_ASSERT(group < landscape->group_count &&
				landscape->group_count <=
					VOE_ASSETS_LANDSCAPE_GROUPS,
			"a group removed past the landscape's groups");

	const uint32_t last = landscape->group_count - 1;
	uint32_t kept = 0;

	for (uint32_t g = group + 1; g <= last; g++)
		landscape->groups[g].seed = (uint8_t)(
			voe_assets_landscape_group_seed(landscape, g) + 1);
	for (uint32_t g = group; g < last; g++) {
		landscape->groups[g] = landscape->groups[g + 1];
		for (uint32_t l = 0; l < VOE_ASSETS_LANDSCAPE_LAYERS; l++)
			landscape->grows[l][g] = landscape->grows[l][g + 1];
	}
	memset(&landscape->groups[last], 0, sizeof(landscape->groups[last]));
	for (uint32_t l = 0; l < VOE_ASSETS_LANDSCAPE_LAYERS; l++)
		landscape->grows[l][last] = 0;
	landscape->group_count = last;

	for (uint32_t c = 0; c < landscape->copy_count; c++) {
		voe_assets_landscape_copy copy = landscape->copies[c];

		if (copy.group == group)
			continue;
		if (copy.group > group)
			copy.group--;
		landscape->copies[kept++] = copy;
	}
	landscape->copy_count = kept;
}
