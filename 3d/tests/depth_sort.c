// THE CLAIM: walking the order this produces visits the objects furthest away
// first, where further away is more negative view-space z.
//
// THE SIGN IS THE ONLY THING WORTH TESTING HERE AND IT IS THE ONLY THING THIS
// CAN GET WRONG. An insertion sort that sorted the other way is still a correct
// sort — it produces a total order, every element appears once, and nothing
// crashes — so no check about permutations or lengths would notice. Every case
// below therefore states which object has to come first by name, and the first
// case would fail loudly with the comparison reversed. That is the failure the
// card warns about: a picture that is right from half the camera angles in a
// scene and wrong from the other half.
//
// EVERY DEPTH HERE IS NEGATIVE, BECAUSE THAT IS WHERE THINGS IN FRONT OF A
// CAMERA ARE. A camera looks along its own −Z (CLAUDE.md), so a scene entirely
// in front of the camera is entirely at negative view-space z and a test using
// positive numbers would be testing an arrangement that cannot occur — and would
// pass with the sign reversed, because reversing the sign of every input and the
// comparison together is the same sort.
//
// IT NEEDS NO GRAPHICS CARD, WHICH IS THE POINT OF THE SORT BEING ITS OWN
// MODULE. The ordering claim on the card is checkable here rather than by
// looking at a window.
#include <3d/depth_sort.h>

#include <testing/test.h>

#include <stdint.h>

// The whole answer for one input: the order, checked element by element.
static void check_order(const float *view_z, uint32_t count,
			const uint32_t *expected)
{
	uint32_t order[8] = { 0 };

	voe_3d_depth_sort(view_z, count, order);
	for (uint32_t i = 0; i < count; i++)
		VOE_TEST_CHECK_INT(order[i], expected[i]);
}

int main(void)
{
	// THE ONE THAT MATTERS. Object 0 is a metre in front of the camera and
	// object 1 is ten metres in front of it, so the far one has to be drawn
	// first and the answer is {1, 0}. Reverse the comparison in the sort and
	// this is {0, 1}.
	{
		const float view_z[2] = { -1.0f, -10.0f };
		const uint32_t expected[2] = { 1, 0 };

		check_order(view_z, 2, expected);
	}

	// Already furthest-first: the sort has to leave it alone rather than
	// turn it round. A sort that reversed its input would pass the case
	// above and fail this one.
	{
		const float view_z[2] = { -10.0f, -1.0f };
		const uint32_t expected[2] = { 0, 1 };

		check_order(view_z, 2, expected);
	}

	// Five, jumbled, so that the answer cannot be produced by a swap. The
	// depths are -3, -9, -1, -5, -7 and furthest first is 1, 4, 3, 0, 2.
	{
		const float view_z[5] = { -3.0f, -9.0f, -1.0f, -5.0f, -7.0f };
		const uint32_t expected[5] = { 1, 4, 3, 0, 2 };

		check_order(view_z, 5, expected);
	}

	// Equal depths keep the order they came in. Two coplanar see-through
	// things have no right answer between them, and an unstable sort would
	// pick a different one as the camera moved — which flickers.
	{
		const float view_z[4] = { -2.0f, -5.0f, -2.0f, -5.0f };
		const uint32_t expected[4] = { 1, 3, 0, 2 };

		check_order(view_z, 4, expected);
	}

	// One object is already sorted, and nothing needs the caller to check
	// for it first.
	{
		const float view_z[1] = { -4.0f };
		const uint32_t expected[1] = { 0 };

		check_order(view_z, 1, expected);
	}

	// Nothing see-through in the scene: no reads, no writes, no branch at
	// the call site. Passing null arrays is what says so.
	voe_3d_depth_sort(NULL, 0, NULL);

	return voe_test_result();
}
