// A landscape's foliage kinds as a set: each kind's seed, and the seed a kind
// added takes (0517 point 4). Called by whatever keys a draw by a kind (the
// grown lattice's hash, the tree pick) and by the editor adding a kind.
//
// A KIND'S SEED IS ITS INDEX UNTIL A KIND IS REMOVED: looks[k].seed is the
// seed plus one and 0 means k, so a file that never named a seed draws as it
// always did, and a kind moved down by a removal keeps the seed it had.
//
// A NEW KIND'S SEED IS ITS OWN INDEX WHEN FREE, else the least seed no kind
// has: there are VOE_ASSETS_LANDSCAPE_KINDS seeds and fewer kinds before the
// new one, so one is always free. Both are scans of at most 16 kinds.
#include <assets/landscape.h>
#include <base/assert.h>

uint32_t voe_assets_landscape_kind_seed(const voe_assets_landscape *landscape,
					uint32_t kind)
{
	VOE_BASE_ASSERT(landscape != NULL, "the kind seed of no landscape");
	VOE_BASE_ASSERT(kind < VOE_ASSETS_LANDSCAPE_KINDS,
			"a kind past the most a landscape holds");

	const uint8_t seed = landscape->looks[kind].seed;

	VOE_BASE_ASSERT(seed <= VOE_ASSETS_LANDSCAPE_KINDS,
			"a seed past the most a landscape holds");
	return seed == 0 ? kind : (uint32_t)seed - 1;
}

uint32_t voe_assets_landscape_seed_new(const voe_assets_landscape *landscape)
{
	VOE_BASE_ASSERT(landscape != NULL, "a new seed in no landscape");
	VOE_BASE_ASSERT(landscape->kind_count < VOE_ASSETS_LANDSCAPE_KINDS,
			"a new seed in a landscape with every kind");

	bool taken[VOE_ASSETS_LANDSCAPE_KINDS] = { false };
	uint32_t seed = 0;

	for (uint32_t k = 0; k < landscape->kind_count; k++)
		taken[voe_assets_landscape_kind_seed(landscape, k)] = true;
	if (!taken[landscape->kind_count])
		return landscape->kind_count;
	while (seed < VOE_ASSETS_LANDSCAPE_KINDS && taken[seed])
		seed++;
	VOE_BASE_ASSERT(seed < VOE_ASSETS_LANDSCAPE_KINDS,
			"no seed free with a kind's room free");
	return seed;
}
