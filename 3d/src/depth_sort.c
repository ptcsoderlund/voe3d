// The sort, and the sign is the whole of it. See the header for why one point
// per object, why this is not in `math`, why a merge sort, and what a reversed
// comparison looks like on screen.
//
// A BOTTOM-UP MERGE SORT OVER `order`, THROUGH `scratch`. Runs of one, then two,
// then four are merged from one array into the other, the two trading places
// each pass; if the last pass left the answer in `scratch` it is copied back.
// No recursion and no allocation: the caller's two arrays are all it touches.
//
// THE COMPARISON IS `<` ON THE INCOMING z AND NOT ON A DISTANCE. Ascending z is
// furthest first, because a camera looks along its own −Z and further away is
// more negative. Writing it as a distance would mean a negation here and a
// second place to get the sign wrong.
//
// STRICTLY LESS, WHICH IS WHAT MAKES IT STABLE. A merge takes from the right
// run only when its depth is strictly less than the left run's, so of two equal
// depths the one that came first is taken first and two coplanar objects keep
// the order the tables held them in.
#include <3d/depth_sort.h>
#include <base/assert.h>

#include <stddef.h>
#include <string.h>

// Merges the sorted runs from[begin, middle) and from[middle, end) into
// into[begin, end), the left run winning ties.
static void merge_runs(const float *view_z, const uint32_t *from,
		       uint32_t *into, uint32_t begin, uint32_t middle,
		       uint32_t end)
{
	uint32_t left = begin;
	uint32_t right = middle;

	for (uint32_t at = begin; at < end; at++) {
		if (right < end &&
		    (left >= middle || view_z[from[right]] < view_z[from[left]]))
			into[at] = from[right++];
		else
			into[at] = from[left++];
	}
}

void voe_3d_depth_sort(const float *view_z, uint32_t count, uint32_t *order,
		       uint32_t *scratch)
{
	VOE_BASE_ASSERT(count == 0 || view_z != NULL,
			"sorting depths that are not there");
	VOE_BASE_ASSERT(count == 0 || order != NULL,
			"sorting into no order array");
	VOE_BASE_ASSERT(count == 0 || scratch != NULL,
			"sorting with no scratch array");

	uint32_t *from = order;
	uint32_t *into = scratch;

	for (uint32_t i = 0; i < count; i++)
		order[i] = i;

	for (uint32_t width = 1; width < count; width *= 2) {
		for (uint32_t begin = 0; begin < count; begin += 2 * width) {
			uint32_t middle = count - begin > width ? begin + width :
								  count;
			uint32_t end = count - middle > width ? middle + width :
								count;

			merge_runs(view_z, from, into, begin, middle, end);
		}

		uint32_t *swap = from;

		from = into;
		into = swap;
	}

	if (from != order)
		memcpy(order, from, (size_t)count * sizeof(*order));
}
