// A landscape's foliage groups as a set: a glb's base size, each group's seed,
// the seed a group added takes, a group added or removed, the glb a copy picks
// and a group's glbs added, replaced and removed (0516, 0517 point 4, 0520,
// 0521). Called by whatever keys a draw by a group (the grown lattice's hash,
// the glb pick), sizes a copy by its glb, and by the editor's group edits.
//
// THE PICK IS LIVE (0521 point 3): a hash of the group's seed and the copy's x
// and z in mm, modulo the shares' total, walked over the glbs in order. Nothing
// of it is stored, so a share edit, a glb added or one removed reaches copies
// already painted; one glb picks 0, equal shares pick as the old `% count`
// did, and all shares 0 pick as all 1. A glb removed leaves the copies be.
//
// AN EDIT IS REFUSED, returning false with nothing changed (0521 point 4): a
// group added at VOE_ASSETS_LANDSCAPE_GROUPS, a glb added at
// VOE_ASSETS_LANDSCAPE_GROUP_GLBS or with a path its group holds, a path
// replaced with one another glb of its group holds. A path too long to keep is
// the caller's to refuse: it is asserted.
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

// The hash 3d's `voe_3d_tree_pick` used (7e86cb12), so equal shares pick as
// its `% count` did.
static uint64_t pick_hash(uint32_t seed, int32_t x_mm, int32_t z_mm)
{
	uint64_t h = (uint64_t)seed * 0x9E3779B97F4A7C15ull ^
		     (uint64_t)(uint32_t)x_mm * 0xC2B2AE3D27D4EB4Full ^
		     (uint64_t)(uint32_t)z_mm * 0x165667B19E3779F9ull;

	h = (h ^ (h >> 33)) * 0xFF51AFD7ED558CCDull;
	h = (h ^ (h >> 33)) * 0xC4CEB9FE1A85EC53ull;
	h ^= h >> 33;
	return h;
}

uint32_t voe_assets_landscape_glb_pick(const voe_assets_landscape *landscape,
				       uint32_t group, int32_t x_mm,
				       int32_t z_mm)
{
	VOE_BASE_ASSERT(landscape != NULL, "a glb picked in no landscape");
	VOE_BASE_ASSERT(group < landscape->group_count &&
				landscape->groups[group].glb_count > 0 &&
				landscape->groups[group].glb_count <=
					VOE_ASSETS_LANDSCAPE_GROUP_GLBS,
			"a glb picked in a group with none");

	const voe_assets_landscape_group *g = &landscape->groups[group];
	uint32_t total = 0;
	uint64_t left;

	for (uint32_t j = 0; j < g->glb_count; j++)
		total += g->looks[j].share;
	left = pick_hash(voe_assets_landscape_group_seed(landscape, group),
			 x_mm, z_mm) %
	       (total == 0 ? g->glb_count : total);
	for (uint32_t j = 0; j < g->glb_count; j++) {
		const uint32_t share = total == 0 ? 1 : g->looks[j].share;

		if (left < share)
			return j;
		left -= share;
	}
	VOE_BASE_ASSERT(false, "a pick past the group's shares");
	return 0;
}

// The glb of `group` holding `path`, or its glb_count when none does.
static uint32_t glb_holding(const voe_assets_landscape_group *group,
			    const char *path)
{
	uint32_t j = 0;

	VOE_BASE_ASSERT(group->glb_count <= VOE_ASSETS_LANDSCAPE_GROUP_GLBS,
			"a group past the most glbs it holds");
	while (j < group->glb_count && strcmp(group->glbs[j], path) != 0)
		j++;
	return j;
}

// `path` into glb `glb` of `group`, asserted to fit.
static void glb_path_set(voe_assets_landscape_group *group, uint32_t glb,
			 const char *path)
{
	const size_t length = strlen(path);

	VOE_BASE_ASSERT(length > 0 && length < VOE_ASSETS_LANDSCAPE_GLB_PATH,
			"a glb path empty or past the room for one");
	memcpy(group->glbs[glb], path, length + 1);
}

bool voe_assets_landscape_group_add(voe_assets_landscape *landscape,
				    const char *path)
{
	VOE_BASE_ASSERT(landscape != NULL && path != NULL,
			"a group added to no landscape or of no path");
	if (landscape->group_count >= VOE_ASSETS_LANDSCAPE_GROUPS)
		return false;

	const uint32_t g = landscape->group_count;
	const uint32_t seed = voe_assets_landscape_seed_new(landscape);
	voe_assets_landscape_group *group = &landscape->groups[g];

	memset(group, 0, sizeof(*group));
	glb_path_set(group, 0, path);
	group->looks[0] = voe_assets_landscape_look_default();
	group->glb_count = 1;
	group->seed = seed == g ? 0 : (uint8_t)(seed + 1);
	landscape->group_count = g + 1;
	return true;
}

bool voe_assets_landscape_glb_add(voe_assets_landscape *landscape,
				  uint32_t group, const char *path)
{
	VOE_BASE_ASSERT(landscape != NULL && path != NULL,
			"a glb added to no landscape or of no path");
	VOE_BASE_ASSERT(group < landscape->group_count,
			"a glb added past the landscape's groups");

	voe_assets_landscape_group *g = &landscape->groups[group];

	if (g->glb_count >= VOE_ASSETS_LANDSCAPE_GROUP_GLBS ||
	    glb_holding(g, path) < g->glb_count)
		return false;
	glb_path_set(g, g->glb_count, path);
	g->looks[g->glb_count] = voe_assets_landscape_look_default();
	g->glb_count++;
	return true;
}

bool voe_assets_landscape_glb_set(voe_assets_landscape *landscape,
				  uint32_t group, uint32_t glb,
				  const char *path)
{
	VOE_BASE_ASSERT(landscape != NULL && path != NULL,
			"a glb replaced in no landscape or by no path");
	VOE_BASE_ASSERT(group < landscape->group_count &&
				glb < landscape->groups[group].glb_count,
			"a glb replaced past its group's glbs");

	voe_assets_landscape_group *g = &landscape->groups[group];
	const uint32_t holder = glb_holding(g, path);

	if (holder < g->glb_count && holder != glb)
		return false;
	glb_path_set(g, glb, path);
	return true;
}

void voe_assets_landscape_glb_remove(voe_assets_landscape *landscape,
				     uint32_t group, uint32_t glb)
{
	VOE_BASE_ASSERT(landscape != NULL, "a glb removed from no landscape");
	VOE_BASE_ASSERT(group < landscape->group_count &&
				landscape->groups[group].glb_count > 1 &&
				landscape->groups[group].glb_count <=
					VOE_ASSETS_LANDSCAPE_GROUP_GLBS &&
				glb < landscape->groups[group].glb_count,
			"a glb removed past its group's or from a group of one");

	voe_assets_landscape_group *g = &landscape->groups[group];
	const uint32_t last = g->glb_count - 1;

	for (uint32_t j = glb; j < last; j++) {
		memcpy(g->glbs[j], g->glbs[j + 1], sizeof(g->glbs[j]));
		g->looks[j] = g->looks[j + 1];
	}
	memset(g->glbs[last], 0, sizeof(g->glbs[last]));
	memset(&g->looks[last], 0, sizeof(g->looks[last]));
	g->glb_count = last;
}
