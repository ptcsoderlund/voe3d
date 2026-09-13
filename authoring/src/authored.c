// The sort and the search over authored ids. The header says why neither is the
// C library's.
#include "authored.h"

#include <string.h>

void voe_authoring_authored_sort(voe_authoring_authored *items,
				 voe_authoring_authored *scratch,
				 uint32_t count)
{
	voe_authoring_authored *from = items;
	voe_authoring_authored *to = scratch;

	// Bottom-up: runs of `width` merged into runs of twice that, the two
	// buffers trading places each round.
	for (uint64_t width = 1; width < count; width *= 2) {
		for (uint64_t low = 0; low < count; low += 2 * width) {
			uint64_t middle = low + width < count ? low + width : count;
			uint64_t high = low + 2 * width < count ? low + 2 * width
								: count;
			uint64_t a = low;
			uint64_t b = middle;

			for (uint64_t out = low; out < high; out++) {
				if (a < middle && (b >= high ||
						   from[a].id <= from[b].id))
					to[out] = from[a++];
				else
					to[out] = from[b++];
			}
		}
		voe_authoring_authored *swap = from;

		from = to;
		to = swap;
	}

	if (from != items)
		memcpy(items, from, count * sizeof(*items));
}

voe_authoring_authored *voe_authoring_authored_find(voe_authoring_authored *items,
						    uint32_t count, uint64_t id)
{
	uint32_t low = 0;
	uint32_t high = count;

	while (low < high) {
		uint32_t middle = low + (high - low) / 2;

		if (items[middle].id == id)
			return &items[middle];
		if (items[middle].id < id)
			low = middle + 1;
		else
			high = middle;
	}
	return NULL;
}
