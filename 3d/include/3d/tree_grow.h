// A tree grown from a species' values (0515 point 2): first its skeleton,
// then one mesh per level, as an in-memory model the store prepares as it
// prepares a read `.glb`. CPU only; no device, no file.
//
//     voe_3d_tree_skeleton skeleton;
//     voe_assets_model model;
//     voe_3d_tree_grow(arena, &values, v, &skeleton);
//     voe_3d_tree_mesh(arena, &skeleton, &values, v, &model);
//
// TWO STEPS, BECAUSE THE SKELETON IS WHERE WIND AND CHOPPING TAKE THE TREE
// LATER: a list of segments with their ends, radii, parent and order, which a
// later step can bend or cut before meshing. It is not stored; the species
// file is all that is kept (0510), and both steps run again on every load.
//
// DETERMINISTIC. Each step draws from its own xorshift seeded from the
// variation alone, so the same values and variation give the same tree on any
// machine and run, and another variation another tree.
//
// COORDINATES: metres, in the tree's own space, right-handed, +Y up, the base
// of the trunk at the origin. The trunk rises `height` with base diameter
// `trunk`; branches leave it above `crown` × its length, `branching` per metre
// of the part above, each `reach` × the parent's length left above it, at
// `angle` degrees from the parent; twigs grow on branches by the same rules.
// `droop` bends a branch or twig toward −Y along its length (negative lifts it)
// and `crooked` jitters every segment's direction. A limb (the trunk, one
// branch, one twig) is consecutive segments, each the parent of the next; its
// first segment's parent is the segment of the limb it leaves, and it starts on
// that segment's axis.
//
// THE LEVELS ARE 0514's: nodes `tree_LOD0`..`tree_LOD3`, each a mesh of a bark
// primitive (material 0) and a leaf primitive (material 1, empty without
// leaves). Level 0 has every limb as a tube and every leaf card. At level k ≥ 1
// twigs are no longer tubes, tube sides and lengthwise segments halve each
// level down to 3 and 1, and every 2^k-th card is kept, scaled by √(2^k) about
// its centre. Tubes have smooth normals, u around the tube in whole metres of
// the limb's base circumference (at least 1) and v along it in metres. A child
// tube adds a collar ring where it leaves its parent's surface, its radius
// halfway from its own to the parent's there, so the joint flares. A card is
// one quad `leaf_size` across, its inner edge on a twig, rolled at random
// about it, `leaf_density` per twig, its normals bent away from the crown's
// centre (the mean of the cards' centres), uv 0–1.
//
// Constraints: growth stops at VOE_3D_TREE_SEGMENTS segments and
// VOE_3D_TREE_LEAVES cards, a whole limb at a time and breadth first (trunk,
// branches, then twigs), so a tree at every key's maximum fits and is never an
// error, though it may then have no twigs or leaves. Raising the caps lifts it
// at the cost of room in the model store. Both steps allocate on the arena
// they are handed and nothing else.
#pragma once

#include <assets/model.h>
#include <assets/tree.h>
#include <base/arena.h>
#include <math/float3.h>

#include <stdint.h>

// The levels each tree has (0514).
#define VOE_3D_TREE_LEVELS 4
// The most segments a skeleton holds and leaf cards a tree carries.
#define VOE_3D_TREE_SEGMENTS 4096
#define VOE_3D_TREE_LEAVES 16384
// The trunk's first segment's parent.
#define VOE_3D_TREE_NO_PARENT UINT32_MAX

// One straight piece of a limb.
typedef struct {
	voe_math_float3 start;
	voe_math_float3 end;
	// Metres, at start and at end.
	float radius_start;
	float radius_end;
	// Into the skeleton's segments, or VOE_3D_TREE_NO_PARENT.
	uint32_t parent;
	// 0 trunk, 1 branch, 2 twig.
	uint32_t order;
} voe_3d_tree_segment;

typedef struct {
	const voe_3d_tree_segment *segments;
	uint32_t count;
} voe_3d_tree_skeleton;

// The skeleton of variation `variation` of `values`, on `arena`.
void voe_3d_tree_grow(voe_base_arena *arena, const voe_assets_tree *values,
		      uint32_t variation, voe_3d_tree_skeleton *out);

// The model of `skeleton`, grown from the same `values` and `variation`, on
// `arena`: two materials, four meshes and four root nodes, one per level.
void voe_3d_tree_mesh(voe_base_arena *arena,
		      const voe_3d_tree_skeleton *skeleton,
		      const voe_assets_tree *values, uint32_t variation,
		      voe_assets_model *out);
