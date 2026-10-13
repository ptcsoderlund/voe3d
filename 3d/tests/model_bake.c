// The bake carries a file's vertex colours: a model built here of one node and
// two primitives on one material, the first with colours (opaque red, then
// half-grey) and the second without, bakes into one part whose first vertices
// carry the colours' complements and whose last carry zeros, opaque white.
//
// Grey 0.5 is (1 − 0.5) × 65535 = 32767.5, which rounds to 32768; the check
// allows one either side.
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

	VOE_TEST_CHECK(voe_3d_model_bake_create(arena, &model, &bake, &error));
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

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	colours_become_complements(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
