// A trunk joint's geometry (ADR-0519), internal to 3d: where a branch grows out
// of the trunk, and the rings of its collar, as arithmetic for tree_mesh.c.
//
//     voe_3d_tree_joint joint;
//     if (voe_3d_tree_joint_of(trunk_segment, branch_segment, &joint))
//             voe_3d_tree_joint_ring(&joint, &stretch, offsets, sides + 1, r,
//                                    own_turns, own_v, share, positions,
//                                    normals, uvs);
//
// A TRUNK JOINT is where a branch (an order-1 limb) leaves the trunk. The
// skeleton starts the branch on the trunk's axis; the mesh starts it instead on
// an ellipse lying on the trunk about J, A along the trunk and B around it, and
// flares it out to the branch's own ring at E, where its plain tube goes on.
// Base vertex k sits on the ellipse in the direction the branch's ring offset
// o_k shows seen along the trunk's normal at J, so it pairs with vertex k of
// every branch ring and the tube does not twist.
//
// J IS SET BACK. J0 is where the branch's line leaves the trunk's round
// surface; E is L along the branch past it and J is L down the trunk from J0,
// at J0's angle. E − J is then L·(t + d), which halves the angle between trunk
// and branch: the collar leaves at θ/2 for every θ and bends out to θ at E,
// and the skeleton, which wind and chopping will take, is untouched.
//
// CHORDS. Ring vertex k at a share s of the flare lies on the straight chord
// from base vertex k to E + r·o_k. The trunk as a level draws it is convex and
// every chord leaves its surface outward, so no collar vertex lies inside it;
// a curve through the plain tube's inside could dip in and bring the crease
// back.
//
// THE TRUNK'S UV, NOT WRAPPED. One weight, 1 − smoothstep(s), blends the
// trunk's radial normal and uv into the branch's own o_k and uv. The trunk's u
// at a point is J's u plus `turns` × the signed angle about the trunk from J,
// at most half a turn, / 2π, so a ring across the trunk's u seam has no jump.
// It may leave 0..turns by a whole number of turns, which bark repeating
// `turns` whole times round the trunk does not show. The angle is read as the
// facets interpolate u: τ × (facet + share across it) / sides, so the base
// ring's u is the trunk mesh's own there. v is the stretch's v at that height.
//
// FAR LEVELS differ only in the snap: J, E, A and B come from the skeleton
// alone, and a level's stretch (its rings, frame and side count) decides only
// where the base ring lands and the trunk normal and uv read there.
//
// Constraints: arithmetic only, no arena. The stretch's facets run on past its
// rings as straight planes, and both rings are taken to share the lower one's
// frame, as a straight stretch's do. A branch past 120° of its trunk is outside
// what the chords were reasoned for.
#pragma once

#include <3d/tree_grow.h>
#include <math/float2.h>
#include <math/float3.h>

#include <stdbool.h>
#include <stdint.h>

// One ring of a trunk tube as a level draws it.
typedef struct {
	voe_math_float3 centre;
	float radius;
	// Metres along the trunk, the ring's bark v.
	float v;
} voe_3d_tree_ring;

// One trunk stretch between two rings as a level draws it.
typedef struct {
	voe_3d_tree_ring lower;
	voe_3d_tree_ring upper;
	// The lower ring's unit frame across the trunk: its vertex at angle a is
	// centre + radius × (cos a × at_0 + sin a × at_90).
	voe_math_float3 at_0;
	voe_math_float3 at_90;
	uint32_t sides;
	// Whole metres of the trunk's base circumference, at least 1: u at the
	// ring's angle a is turns × a / τ.
	float turns;
} voe_3d_tree_stretch;

// One trunk joint, from the skeleton alone (ADR-0519 points 1–2).
typedef struct {
	// J, on the trunk's round surface, and E, the branch's first plain ring.
	voe_math_float3 centre;
	voe_math_float3 end;
	// The trunk's unit direction t, and the unit direction around it at J.
	voe_math_float3 along;
	voe_math_float3 around;
	// The base ellipse's half-axes A along t and B around, metres.
	float half_along;
	float half_around;
} voe_3d_tree_joint;

// The joint of `branch`'s first segment on `trunk`; false when that segment
// never leaves the trunk's round surface.
bool voe_3d_tree_joint_of(voe_3d_tree_segment trunk, voe_3d_tree_segment branch,
			  voe_3d_tree_joint *out);

// One collar ring of `count` vertices (sides + 1, the last the seam) at `share`
// 0–1 of the flare: 0 the base ring on `stretch`'s facets, 1 the branch's
// plain ring of `radius` about E. `offsets` are the branch's unit ring offsets
// at E; its own u is `own_turns` × k / sides, its own v `own_v`.
void voe_3d_tree_joint_ring(const voe_3d_tree_joint *joint,
			    const voe_3d_tree_stretch *stretch,
			    const voe_math_float3 *offsets, uint32_t count,
			    float radius, float own_turns, float own_v,
			    float share, voe_math_float3 *positions,
			    voe_math_float3 *normals, voe_math_float2 *uvs);
