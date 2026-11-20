// A tree grown by 3d/tree_grow.h from the species defaults: the same values
// and variation give equal skeletons and another variation another; every
// child segment starts on its parent, at the defaults and at every key's
// maximum, within the cap, and that tree meshes; each level has fewer triangles than the one
// before; leaf_density 0 gives no leaf triangles; every vertex lies within a
// box of `height` about the trunk; and the nodes are tree_LOD0..tree_LOD3.
// The pick gives the same variation for the same kind and place, and 6
// variations over a grid of 10 000 copies each take 12 to 22 % of them.
// Needs no graphics card.
#include <3d/tree_grow.h>
#include <assets/model.h>
#include <assets/tree.h>
#include <base/arena.h>
#include <math/float3.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ARENA (16 * 1024 * 1024)

static bool same_skeleton(voe_3d_tree_skeleton a, voe_3d_tree_skeleton b)
{
	return a.count == b.count &&
	       memcmp(a.segments, b.segments, sizeof(*a.segments) * a.count) == 0;
}

static void a_variation_repeats_and_another_differs(voe_base_arena *arena)
{
	const voe_assets_tree values = voe_assets_tree_default();
	voe_3d_tree_skeleton a, b, c;

	voe_3d_tree_grow(arena, &values, 3, &a);
	voe_3d_tree_grow(arena, &values, 3, &b);
	voe_3d_tree_grow(arena, &values, 4, &c);
	VOE_TEST_CHECK(a.count > 8);
	VOE_TEST_CHECK(same_skeleton(a, b));
	VOE_TEST_CHECK(!same_skeleton(a, c));
}

// The distance from `p` to the segment `s`.
static float off_segment(voe_math_float3 p, voe_3d_tree_segment s)
{
	const voe_math_float3 d = voe_math_float3_sub(s.end, s.start);
	float t = voe_math_float3_dot(voe_math_float3_sub(p, s.start), d) /
		  voe_math_float3_dot(d, d);

	t = fminf(fmaxf(t, 0.0f), 1.0f);
	return voe_math_float3_length(voe_math_float3_sub(
		p, voe_math_float3_add(s.start, voe_math_float3_scale(d, t))));
}

static bool children_start_on_parents(voe_3d_tree_skeleton s, float height)
{
	bool on = s.segments[0].parent == VOE_3D_TREE_NO_PARENT;

	for (uint32_t i = 1; i < s.count; i++) {
		const uint32_t p = s.segments[i].parent;

		on &= p < i && off_segment(s.segments[i].start,
					   s.segments[p]) <= 1e-5f * height;
	}
	return on;
}

static void children_start_on_their_parents(voe_base_arena *arena)
{
	voe_assets_tree values = voe_assets_tree_default();
	voe_3d_tree_skeleton s;
	voe_assets_model m;
	bool twigs = false;

	voe_3d_tree_grow(arena, &values, 0, &s);
	for (uint32_t i = 0; i < s.count; i++)
		twigs |= s.segments[i].order == 2;
	VOE_TEST_CHECK(twigs);
	VOE_TEST_CHECK(children_start_on_parents(s, values.height));

	values.height = VOE_ASSETS_TREE_HEIGHT_MAX;
	values.trunk = VOE_ASSETS_TREE_TRUNK_MAX;
	values.crown = VOE_ASSETS_TREE_CROWN_MAX;
	values.branching = VOE_ASSETS_TREE_BRANCHING_MAX;
	values.reach = VOE_ASSETS_TREE_REACH_MAX;
	values.angle = VOE_ASSETS_TREE_ANGLE_MAX;
	values.droop = VOE_ASSETS_TREE_DROOP_MAX;
	values.crooked = VOE_ASSETS_TREE_CROOKED_MAX;
	values.leaf_size = VOE_ASSETS_TREE_LEAF_SIZE_MAX;
	values.leaf_density = VOE_ASSETS_TREE_LEAF_DENSITY_MAX;
	voe_3d_tree_grow(arena, &values, 0, &s);
	VOE_TEST_CHECK(s.count <= VOE_3D_TREE_SEGMENTS);
	VOE_TEST_CHECK(children_start_on_parents(s, values.height));
	voe_3d_tree_mesh(arena, &s, &values, 0, &m);
	VOE_TEST_CHECK(m.primitives[0].index_count > 0);
}

static uint32_t triangles(const voe_assets_model *m, uint32_t level)
{
	const voe_assets_mesh mesh = m->meshes[m->nodes[level].mesh];
	uint32_t n = 0;

	for (uint32_t p = 0; p < mesh.primitive_count; p++)
		n += m->primitives[mesh.first_primitive + p].index_count / 3;
	return n;
}

static voe_assets_model grown(const voe_assets_tree *values,
			      voe_base_arena *arena)
{
	voe_3d_tree_skeleton s;
	voe_assets_model m;

	voe_3d_tree_grow(arena, values, 1, &s);
	voe_3d_tree_mesh(arena, &s, values, 1, &m);
	return m;
}

static void each_level_has_fewer_triangles(voe_base_arena *arena)
{
	const voe_assets_tree values = voe_assets_tree_default();
	const voe_assets_model m = grown(&values, arena);

	VOE_TEST_CHECK_INT(m.node_count, VOE_3D_TREE_LEVELS);
	for (uint32_t k = 0; k + 1 < VOE_3D_TREE_LEVELS; k++)
		VOE_TEST_CHECK(triangles(&m, k + 1) < triangles(&m, k));
}

static void no_leaf_density_no_leaves(voe_base_arena *arena)
{
	voe_assets_tree values = voe_assets_tree_default();
	voe_assets_model m = grown(&values, arena);

	VOE_TEST_CHECK(m.primitives[1].index_count > 0);
	values.leaf_density = 0.0f;
	m = grown(&values, arena);
	for (uint32_t k = 0; k < VOE_3D_TREE_LEVELS; k++) {
		VOE_TEST_CHECK_INT(m.primitives[2 * k + 1].material, 1);
		VOE_TEST_CHECK_INT(m.primitives[2 * k + 1].index_count, 0);
		VOE_TEST_CHECK(m.primitives[2 * k].index_count > 0);
	}
}

// Within `height` of the trunk's axis across, and from `height` / 2 below
// its base to `height` / 2 above its top.
static void every_vertex_in_a_box_about_the_trunk(voe_base_arena *arena)
{
	const voe_assets_tree values = voe_assets_tree_default();
	const voe_assets_model m = grown(&values, arena);
	const float h = values.height;
	bool inside = true;

	for (uint32_t p = 0; p < m.primitive_count; p++) {
		const voe_assets_primitive prim = m.primitives[p];

		for (uint32_t v = 0; v < prim.vertex_count; v++) {
			const voe_math_float3 at = prim.positions[v];

			inside &= fabsf(at.x) <= h && fabsf(at.z) <= h &&
				  at.y >= -0.5f * h && at.y <= 1.5f * h;
		}
	}
	VOE_TEST_CHECK(inside);
}

static void the_nodes_are_named_by_level(voe_base_arena *arena)
{
	const voe_assets_tree values = voe_assets_tree_default();
	const voe_assets_model m = grown(&values, arena);
	static const char *const names[VOE_3D_TREE_LEVELS] = {
		"tree_LOD0", "tree_LOD1", "tree_LOD2", "tree_LOD3"
	};

	VOE_TEST_CHECK_INT(m.root_count, VOE_3D_TREE_LEVELS);
	VOE_TEST_CHECK_INT(m.material_count, 2);
	for (uint32_t k = 0; k < VOE_3D_TREE_LEVELS; k++) {
		VOE_TEST_CHECK(strcmp(m.nodes[m.roots[k]].name.text, names[k]) ==
			       0);
		VOE_TEST_CHECK_INT(m.meshes[m.nodes[k].mesh].primitive_count,
				   2);
	}
}

static void the_pick_repeats(void)
{
	for (int32_t i = -50; i < 50; i++) {
		const int32_t x = i * 1733, z = -i * 911;

		VOE_TEST_CHECK_INT(voe_3d_tree_pick(3, x, z, 6),
				   voe_3d_tree_pick(3, x, z, 6));
		VOE_TEST_CHECK(voe_3d_tree_pick(3, x, z, 6) < 6);
	}
}

// A 100 × 100 grid of copies 1.5 m apart, as a planted wood stands.
static void the_pick_spreads_evenly(void)
{
	uint32_t shares[6] = { 0 };

	for (int32_t i = 0; i < 100; i++)
		for (int32_t j = 0; j < 100; j++)
			shares[voe_3d_tree_pick(2, i * 1500 - 75000,
						j * 1500 - 75000, 6)]++;
	for (uint32_t v = 0; v < 6; v++) {
		printf("variation %u: %u of 10000\n", v, shares[v]);
		VOE_TEST_CHECK(shares[v] >= 1200 && shares[v] <= 2200);
	}
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);

	the_pick_repeats();
	the_pick_spreads_evenly();

	a_variation_repeats_and_another_differs(arena);
	voe_base_arena_clear(arena);
	children_start_on_their_parents(arena);
	voe_base_arena_clear(arena);
	each_level_has_fewer_triangles(arena);
	voe_base_arena_clear(arena);
	no_leaf_density_no_leaves(arena);
	voe_base_arena_clear(arena);
	every_vertex_in_a_box_about_the_trunk(arena);
	voe_base_arena_clear(arena);
	the_nodes_are_named_by_level(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
