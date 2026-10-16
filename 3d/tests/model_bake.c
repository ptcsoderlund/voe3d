// The bake carries a file's vertex colours: a model built here of one node and
// two primitives on one material, the first with colours (opaque red, then
// half-grey) and the second without, bakes into one part whose first vertices
// carry the colours' complements and whose last carry zeros, opaque white.
// Red's complement is 0/65535/65535/0.
//
// Grey 0.5 is (1 − 0.5) × 65535 = 32767.5, which rounds to 32768; the check
// allows one either side.
//
// The bake takes one level of a file's own: a root "Tree" with children
// "Tree_LOD1", "Leaves_lod2" (over an unnamed child, which inherits level 2) and
// "Twig_LOD7" counts 3 levels, bakes the root alone at level 0, one part at
// level 1 and materials 1 then 0 at level 2, level 7 left out; a tree with no
// suffix counts 1 and bakes every node at level 0. A root "Rock" with children
// "Rock_LOD5" and "Twig_LOD7", one shared triangle each, counts 6: level 5 is
// the last a file may name, level 7 still left out.
//
// It reads the internal bake through model_bake.h. Needs no graphics card.
#include "../src/model_bake.h"

#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>

#include <testing/test.h>

#define SCRATCH (64 * 1024)

static const voe_math_float3 POSITIONS[3] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 1.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f },
};
static const voe_math_float4 COLOURS[3] = {
	{ 1.0f, 0.0f, 0.0f, 1.0f },
	{ 0.5f, 0.5f, 0.5f, 1.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f },
};
static const uint32_t INDICES[3] = { 0, 1, 2 };

static void check_complement(const voe_render_vertex *vertex, int r, int g,
			     int b, int a)
{
	VOE_TEST_CHECK_INT(vertex->colour_complement[0], r);
	VOE_TEST_CHECK_INT(vertex->colour_complement[1], g);
	VOE_TEST_CHECK_INT(vertex->colour_complement[2], b);
	VOE_TEST_CHECK_INT(vertex->colour_complement[3], a);
}

static void colours_become_complements(voe_base_arena *arena)
{
	const voe_assets_primitive primitives[2] = {
		{ .positions = POSITIONS, .colours = COLOURS, .vertex_count = 3,
		  .indices = INDICES, .index_count = 3, .material = 0 },
		{ .positions = POSITIONS, .vertex_count = 3, .indices = INDICES,
		  .index_count = 3, .material = 0 },
	};
	const voe_assets_mesh mesh = { .first_primitive = 0,
				       .primitive_count = 2 };
	const voe_assets_material material = { .base_colour = { 1, 1, 1, 1 } };
	const voe_assets_node node = { .local = voe_math_float4x4_identity(),
				       .mesh = 0 };
	const uint32_t root = 0;
	const voe_assets_model model = {
		.primitives = primitives, .primitive_count = 2,
		.meshes = &mesh, .mesh_count = 1,
		.materials = &material, .material_count = 1,
		.nodes = &node, .node_count = 1,
		.roots = &root, .root_count = 1,
	};
	voe_base_error error = VOE_BASE_OK;
	voe_3d_model_bake bake;
	const voe_render_vertex *grey;

	VOE_TEST_CHECK(voe_3d_model_bake_create(arena, &model, 0, &bake, &error));
	VOE_TEST_CHECK_INT(bake.part_count, 1);
	VOE_TEST_CHECK_INT(bake.vertex_count, 6);
	if (bake.vertex_count != 6)
		return;

	check_complement(&bake.vertices[0], 0, 65535, 65535, 0);
	check_complement(&bake.vertices[2], 0, 65535, 65535, 0);
	grey = &bake.vertices[1];
	for (int c = 0; c < 3; c++) {
		VOE_TEST_CHECK(grey->colour_complement[c] >= 32767 &&
			       grey->colour_complement[c] <= 32768);
	}
	VOE_TEST_CHECK_INT(grey->colour_complement[3], 0);
	for (uint32_t v = 3; v < 6; v++)
		check_complement(&bake.vertices[v], 0, 0, 0, 0);
}

static void levels_are_told_by_names(voe_base_arena *arena)
{
	const voe_assets_primitive primitives[2] = {
		{ .positions = POSITIONS, .vertex_count = 3, .indices = INDICES,
		  .index_count = 3, .material = 0 },
		{ .positions = POSITIONS, .vertex_count = 3, .indices = INDICES,
		  .index_count = 3, .material = 1 },
	};
	const voe_assets_mesh meshes[2] = {
		{ .first_primitive = 0, .primitive_count = 1 },
		{ .first_primitive = 1, .primitive_count = 1 },
	};
	const voe_assets_material materials[2] = {
		{ .base_colour = { 1, 1, 1, 1 } },
		{ .base_colour = { 0, 1, 0, 1 } },
	};
	const uint32_t tree_children[3] = { 1, 2, 4 };
	const uint32_t leaves_children[1] = { 3 };
	const voe_assets_node nodes[5] = {
		{ .local = voe_math_float4x4_identity(), .mesh = 0,
		  .children = tree_children, .child_count = 3,
		  .name = { "Tree" } },
		{ .local = voe_math_float4x4_identity(), .mesh = 0,
		  .name = { "Tree_LOD1" } },
		{ .local = voe_math_float4x4_identity(), .mesh = 1,
		  .children = leaves_children, .child_count = 1,
		  .name = { "Leaves_lod2" } },
		{ .local = voe_math_float4x4_identity(), .mesh = 0 },
		{ .local = voe_math_float4x4_identity(), .mesh = 0,
		  .name = { "Twig_LOD7" } },
	};
	const uint32_t root = 0;
	voe_assets_model model = {
		.primitives = primitives, .primitive_count = 2,
		.meshes = meshes, .mesh_count = 2,
		.materials = materials, .material_count = 2,
		.nodes = nodes, .node_count = 5,
		.roots = &root, .root_count = 1,
	};
	const uint32_t plain_children[1] = { 1 };
	const voe_assets_node plain[2] = {
		{ .local = voe_math_float4x4_identity(), .mesh = 0,
		  .children = plain_children, .child_count = 1,
		  .name = { "Tree" } },
		{ .local = voe_math_float4x4_identity(), .mesh = 0,
		  .name = { "Trunk" } },
	};
	voe_base_error error = VOE_BASE_OK;
	voe_3d_model_bake bake;

	VOE_TEST_CHECK_INT(voe_3d_model_bake_levels(&model), 3);

	VOE_TEST_CHECK(voe_3d_model_bake_create(arena, &model, 0, &bake, &error));
	VOE_TEST_CHECK_INT(bake.part_count, 1);
	VOE_TEST_CHECK_INT(bake.vertex_count, 3);

	VOE_TEST_CHECK(voe_3d_model_bake_create(arena, &model, 1, &bake, &error));
	VOE_TEST_CHECK_INT(bake.part_count, 1);
	VOE_TEST_CHECK_INT(bake.parts[0].material, 0);

	VOE_TEST_CHECK(voe_3d_model_bake_create(arena, &model, 2, &bake, &error));
	VOE_TEST_CHECK_INT(bake.part_count, 2);
	VOE_TEST_CHECK_INT(bake.parts[0].material, 1);
	VOE_TEST_CHECK_INT(bake.parts[1].material, 0);

	model.nodes = plain;
	model.node_count = 2;
	VOE_TEST_CHECK_INT(voe_3d_model_bake_levels(&model), 1);
	VOE_TEST_CHECK(voe_3d_model_bake_create(arena, &model, 0, &bake, &error));
	VOE_TEST_CHECK_INT(bake.part_count, 1);
	VOE_TEST_CHECK_INT(bake.vertex_count, 6);
}

static void the_last_level_is_five(void)
{
	const voe_assets_primitive primitive = {
		.positions = POSITIONS, .vertex_count = 3, .indices = INDICES,
		.index_count = 3, .material = 0,
	};
	const voe_assets_mesh mesh = { .first_primitive = 0,
				       .primitive_count = 1 };
	const voe_assets_material material = { .base_colour = { 1, 1, 1, 1 } };
	const uint32_t rock_children[2] = { 1, 2 };
	const voe_assets_node nodes[3] = {
		{ .local = voe_math_float4x4_identity(), .mesh = 0,
		  .children = rock_children, .child_count = 2,
		  .name = { "Rock" } },
		{ .local = voe_math_float4x4_identity(), .mesh = 0,
		  .name = { "Rock_LOD5" } },
		{ .local = voe_math_float4x4_identity(), .mesh = 0,
		  .name = { "Twig_LOD7" } },
	};
	const uint32_t root = 0;
	const voe_assets_model model = {
		.primitives = &primitive, .primitive_count = 1,
		.meshes = &mesh, .mesh_count = 1,
		.materials = &material, .material_count = 1,
		.nodes = nodes, .node_count = 3,
		.roots = &root, .root_count = 1,
	};

	VOE_TEST_CHECK_INT(voe_3d_model_bake_levels(&model), 6);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	colours_become_complements(arena);
	levels_are_told_by_names(arena);
	the_last_level_is_five();
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
