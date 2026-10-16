// How far apart two bakes are, on bakes built by hand: a unit quad against
// itself is 0; against the same quad 0.25 higher, 0.25; a 2 × 2 quad against a
// 1 × 1 quad in its corner, both flat, the big quad's far corner to the small
// one's, √2, either order; and a 32 × 32 grid of quads against a copy with one
// vertex lifted 0.5, 0.5, which the grid's search has to find among 2048
// triangles.
//
// It reads the internal measure through measure.h. Needs no graphics card.
#include "../src/measure.h"

#include <base/arena.h>

#include <testing/test.h>

#include <math.h>
#include <stdint.h>

#define SCRATCH (1024 * 1024)
#define GRID 32

// A flat grid of `cells` × `cells` quads of side `size` on y = `height`, one
// part, two triangles a quad.
static voe_3d_model_bake grid_bake(voe_base_arena *arena, uint32_t cells,
				   float size, float height)
{
	const uint32_t row = cells + 1;
	voe_3d_model_bake bake = { 0 };
	uint32_t written = 0;

	bake.vertex_count = row * row;
	bake.index_count = cells * cells * 6;
	bake.vertices = voe_base_arena_push(arena, sizeof *bake.vertices *
							   bake.vertex_count);
	bake.indices = voe_base_arena_push(arena, sizeof *bake.indices *
							  bake.index_count);
	for (uint32_t z = 0; z < row; z++) {
		for (uint32_t x = 0; x < row; x++) {
			bake.vertices[z * row + x].position = (voe_math_float3){
				size * (float)x / (float)cells, height,
				size * (float)z / (float)cells
			};
		}
	}
	for (uint32_t z = 0; z < cells; z++) {
		for (uint32_t x = 0; x < cells; x++) {
			const uint32_t corner = z * row + x;
			const uint32_t quad[6] = { corner, corner + row, corner + 1,
						   corner + 1, corner + row,
						   corner + row + 1 };

			for (int i = 0; i < 6; i++)
				bake.indices[written++] = quad[i];
		}
	}
	bake.parts[0] = (voe_3d_model_bake_part){
		.vertex_count = bake.vertex_count,
		.index_count = bake.index_count,
	};
	bake.part_count = 1;
	return bake;
}

static void same_bake_is_nought(voe_base_arena *arena)
{
	const voe_3d_model_bake quad = grid_bake(arena, 1, 1.0f, 0.0f);

	VOE_TEST_CHECK_FLOAT(voe_3d_measure_apart(arena, &quad, &quad), 0.0f,
			     0.0f);
}

static void lifted_quad_is_its_lift(voe_base_arena *arena)
{
	const voe_3d_model_bake quad = grid_bake(arena, 1, 1.0f, 0.0f);
	const voe_3d_model_bake lifted = grid_bake(arena, 1, 1.0f, 0.25f);

	VOE_TEST_CHECK_FLOAT(voe_3d_measure_apart(arena, &quad, &lifted), 0.25f,
			     1e-6f);
}

static void small_inside_big_is_the_far_corner(voe_base_arena *arena)
{
	const voe_3d_model_bake big = grid_bake(arena, 1, 2.0f, 0.0f);
	const voe_3d_model_bake small = grid_bake(arena, 1, 1.0f, 0.0f);

	VOE_TEST_CHECK_FLOAT(voe_3d_measure_apart(arena, &big, &small),
			     sqrtf(2.0f), 1e-5f);
	VOE_TEST_CHECK_FLOAT(voe_3d_measure_apart(arena, &small, &big),
			     sqrtf(2.0f), 1e-5f);
}

static void grid_finds_far_triangles(voe_base_arena *arena)
{
	const voe_3d_model_bake flat = grid_bake(arena, GRID, GRID, 0.0f);
	voe_3d_model_bake moved = grid_bake(arena, GRID, GRID, 0.0f);

	moved.vertices[7 * (GRID + 1) + 23].position.y = 0.5f;
	VOE_TEST_CHECK_FLOAT(voe_3d_measure_apart(arena, &flat, &moved), 0.5f,
			     1e-5f);
	VOE_TEST_CHECK_FLOAT(voe_3d_measure_apart(arena, &moved, &flat), 0.5f,
			     1e-5f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	same_bake_is_nought(arena);
	lifted_quad_is_its_lift(arena);
	small_inside_big_is_the_far_corner(arena);
	grid_finds_far_triangles(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
