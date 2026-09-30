// The order blended objects have to be drawn in, and the only place in the
// engine where "which of these is further away" is written down as arithmetic.
//
// BLENDING IS NOT COMMUTATIVE, WHICH IS THE WHOLE REASON THIS EXISTS. Two solid
// objects may be drawn in either order because the depth buffer decides per
// pixel which one survives; two see-through ones may not, because both survive
// and what comes out depends on which was multiplied into the target first. So
// the blended pass draws furthest away first and this is what says which that
// is. The pass that makes the order load-bearing is the one that does not write
// depth — see render/include/render/device.h.
//
// THE KEY IS ONE POINT PER OBJECT: THE VIEW-SPACE DEPTH OF ITS ORIGIN. Not per
// triangle and not per fragment; those are order-independent transparency and
// per-fragment sorting, and both are refused by name on the card that brought
// this in. One point per object is wrong for two objects that interpenetrate and
// for a long one seen end-on, and that is the trade being taken rather than an
// oversight.
//
// FURTHER AWAY IS MORE NEGATIVE, AND GETTING THAT BACKWARDS IS THIS FILE'S ONE
// REAL BUG. This engine is right-handed with −Z forward (CLAUDE.md), so a camera
// looks along its own −Z and something twice as far away has twice as negative a
// view-space z. Sorting the other way produces a picture that is correct from
// half the angles in a scene and wrong from the other half, which is exactly the
// kind of wrong nobody notices for a week. 3d/tests/depth_sort.c is the claim.
//
// IT IS HERE AND NOT IN `math` FOR THE REASON voe_3d_normal_matrix IS. `math`
// knows nothing about cameras or about what a draw order is for; this is a fact
// about how a frame is assembled, and this folder is the one that knows it. It
// is a function rather than a loop inside the draw system so that it has
// somewhere to be tested — the ordering claim is checkable on a machine with no
// graphics card, and that is where it is checked.
#pragma once

#include <stdint.h>

// Fills `order` with the numbers 0 to count-1, arranged so that walking it
// visits the view-space depths in `view_z` furthest away first — ascending z,
// because further away is more negative.
//
// `order` IS AN OUTPUT AND NOT AN INPUT. It is written from nothing; whatever
// was in it is ignored. `order` and `scratch` are the caller's arrays of `count`
// elements each, `scratch` only worked in, and this still allocates nothing.
//
// EQUAL DEPTHS KEEP THE ORDER THEY CAME IN, which is what makes a frame with two
// coplanar see-through things look the same as the frame before it rather than
// flickering between two answers as an unstable sort reshuffles them. The merge
// sort stays stable for that reason.
//
// A count of zero does nothing, which is what a scene with nothing see-through
// in it wants — the caller need not branch. The sort is a merge sort and not
// the insertion sort it was: particles put thousands of blended objects into one
// pass, rebuilt unsorted every frame (ADR-0298), and insertion sort is quadratic
// on exactly that. Merge sort is n log n on any input, and the price is the
// scratch array.
void voe_3d_depth_sort(const float *view_z, uint32_t count, uint32_t *order,
		       uint32_t *scratch);
