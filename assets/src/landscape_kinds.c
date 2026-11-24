// A landscape's foliage kinds as a set: each kind's seed, the seed a kind
// added takes, and a kind removed (0516, 0517 point 4). Called by whatever keys
// a draw by a kind (the grown lattice's hash, the tree pick) and by the editor
// adding or removing a kind.
//
// A REMOVAL DROPS THE KIND'S PATH, LOOK, EVERY LAYER'S GROWTH OF IT AND ITS
// COPIES; later kinds move down one with their own, their copies keep their
// order with the kind lowered, and the freed last slot is cleared. LATER KINDS
// KEEP THEIR SEEDS: each is made explicit before anything moves, so their
// draws stay where they were. Nothing allocates: the copies close up in place.
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

#include <string.h>

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

void voe_assets_landscape_kind_remove(voe_assets_landscape *landscape,
				      uint32_t kind)
{
	VOE_BASE_ASSERT(landscape != NULL, "a kind removed from no landscape");
	VOE_BASE_ASSERT(kind < landscape->kind_count &&
				landscape->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"a kind removed past the landscape's kinds");

	const uint32_t last = landscape->kind_count - 1;
	uint32_t kept = 0;

	for (uint32_t k = kind + 1; k <= last; k++)
		landscape->looks[k].seed =
			(uint8_t)(voe_assets_landscape_kind_seed(landscape, k) + 1);
	for (uint32_t k = kind; k < last; k++) {
		memcpy(landscape->kinds[k], landscape->kinds[k + 1],
		       sizeof(landscape->kinds[k]));
		landscape->looks[k] = landscape->looks[k + 1];
		for (uint32_t l = 0; l < VOE_ASSETS_LANDSCAPE_LAYERS; l++)
			landscape->grows[l][k] = landscape->grows[l][k + 1];
	}
	memset(landscape->kinds[last], 0, sizeof(landscape->kinds[last]));
	landscape->looks[last] = (voe_assets_landscape_look){ 0 };
	for (uint32_t l = 0; l < VOE_ASSETS_LANDSCAPE_LAYERS; l++)
		landscape->grows[l][last] = 0;
	landscape->kind_count = last;

	for (uint32_t c = 0; c < landscape->copy_count; c++) {
		voe_assets_landscape_copy copy = landscape->copies[c];

		if (copy.kind == kind)
			continue;
		if (copy.kind > kind)
			copy.kind--;
		landscape->copies[kept++] = copy;
	}
	landscape->copy_count = kept;
}
