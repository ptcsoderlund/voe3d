// The sort, and the sign is the whole of it. See the header for why one point
// per object, why this is not in `math`, and what a reversed comparison looks
// like on screen.
//
// THE COMPARISON IS `<` ON THE INCOMING z AND NOT ON A DISTANCE. Ascending z is
// furthest first, because a camera looks along its own −Z and further away is
// more negative. Writing it as a distance would mean a negation here and a
// second place to get the sign wrong.
//
// STRICTLY LESS, WHICH IS WHAT MAKES IT STABLE. The scan below stops at the
// first element that is not strictly greater than the one being placed, so an
// equal depth is left where it already was and two coplanar objects keep the
// order the tables held them in.
#include <3d/depth_sort.h>
#include <base/assert.h>

#include <stddef.h>

void voe_3d_depth_sort(const float *view_z, uint32_t count, uint32_t *order)
{
	VOE_BASE_ASSERT(count == 0 || view_z != NULL,
			"sorting depths that are not there");
	VOE_BASE_ASSERT(count == 0 || order != NULL,
			"sorting into no order array");

	for (uint32_t i = 0; i < count; i++) {
		uint32_t placing = i;
		uint32_t at = i;

		// Walk back over everything already placed that is nearer than
		// this one, shifting it along, and drop this one in the gap.
		while (at > 0 && view_z[order[at - 1]] > view_z[placing]) {
			order[at] = order[at - 1];
			at--;
		}
		order[at] = placing;
	}
}
