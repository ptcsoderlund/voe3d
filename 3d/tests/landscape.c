// The landscape arithmetic of 3d/landscape.h: the bilinear height between four
// heights, a ray meeting a raised cell and missing beside the grid, raise,
// smooth and flatten by 0379 point 3's rates, a chunk's quads wound up, and a
// rect on a chunk edge touching both chunks. Needs no graphics card.
#include <3d/landscape.h>
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <math.h>

#define ARENA (1024 * 1024)

static float *height_at(voe_assets_landscape *land, uint32_t c, uint32_t r)
{
	return &land->heights[(size_t)r * (land->cells + 1) + c];
}

// 16 m with 4 cells puts a height every 4 m; (-3, -2) is a quarter along x
// and half along z between heights 1, 2, 3 and 4. Off the grid it clamps.
static void height_is_bilinear_between_four(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 4, arena);

	*height_at(&land, 1, 1) = 1.0f;
	*height_at(&land, 2, 1) = 2.0f;
	*height_at(&land, 1, 2) = 3.0f;
	*height_at(&land, 2, 2) = 4.0f;
	VOE_TEST_CHECK_FLOAT(voe_3d_landscape_height(&land, -3.0f, -2.0f),
			     2.25f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(voe_3d_landscape_height(&land, -4.0f, -4.0f),
			     1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(voe_3d_landscape_height(&land, 100.0f, 0.0f),
			     0.0f, 1e-5f);
}

// A 4 m peak in the middle: straight down from 10 m meets it 6 m on, and a
// level ray at 1 m from the side meets its slope where it is 1 m, 3 m short
// of the middle.
static void ray_meets_a_raised_cell(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 4, arena);
	float distance = 0.0f;

	*height_at(&land, 2, 2) = 4.0f;
	VOE_TEST_CHECK(voe_3d_landscape_ray(&land, (voe_math_float3){ 0, 10, 0 },
					    (voe_math_float3){ 0, -1, 0 },
					    &distance));
	VOE_TEST_CHECK_FLOAT(distance, 6.0f, 1e-3f);
	VOE_TEST_CHECK(voe_3d_landscape_ray(&land,
					    (voe_math_float3){ -20, 1, 0 },
					    (voe_math_float3){ 1, 0, 0 },
					    &distance));
	VOE_TEST_CHECK_FLOAT(distance, 17.0f, 1e-3f);
}

// Down beside the grid, and level above its highest height, meet nothing.
static void ray_beside_the_grid_misses(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 4, arena);
	float distance = -1.0f;

	*height_at(&land, 2, 2) = 4.0f;
	VOE_TEST_CHECK(!voe_3d_landscape_ray(&land,
					     (voe_math_float3){ 20, 10, 0 },
					     (voe_math_float3){ 0, -1, 0 },
					     &distance));
	VOE_TEST_CHECK(!voe_3d_landscape_ray(&land,
					     (voe_math_float3){ -20, 5, 0 },
					     (voe_math_float3){ 1, 0, 0 },
					     &distance));
	VOE_TEST_CHECK_FLOAT(distance, -1.0f, 0.0f);
}

// Radius 16, softness 0.5, strength 1, one second clamped to 0.1: the middle
// rises 1·16·0.5·0.1 = 0.8 m, 12 m out less, the rim at 16 m not at all.
static void raise_lifts_the_middle_most_and_the_rim_not_at_all(
	voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(64.0f, 16, arena);
	const voe_3d_brush brush = { .kind = VOE_3D_BRUSH_RAISE,
				     .radius = 16.0f,
				     .strength = 1.0f,
				     .softness = 0.5f };
	const voe_3d_landscape_rect rect =
		voe_3d_landscape_brush(&land, &brush, 0.0f, 0.0f, 1.0f, arena);

	VOE_TEST_CHECK_FLOAT(*height_at(&land, 8, 8), 0.8f, 1e-5f);
	VOE_TEST_CHECK(*height_at(&land, 11, 8) > 0.0f);
	VOE_TEST_CHECK(*height_at(&land, 11, 8) < 0.8f);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 12, 8), 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 8, 4), 0.0f, 0.0f);
	VOE_TEST_CHECK(rect.x0 >= 4 && rect.x0 <= 8 && rect.x1 > 8 &&
		       rect.x1 <= 13);
	VOE_TEST_CHECK(rect.z0 >= 4 && rect.z0 <= 8 && rect.z1 > 8 &&
		       rect.z1 <= 13);
}

// A 10 m spike under a full smooth goes to its neighbours' mean, 0; a
// neighbour rises toward its own mean, the spike counted in it.
static void smooth_lowers_a_spike(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(64.0f, 16, arena);
	const voe_3d_brush brush = { .kind = VOE_3D_BRUSH_SMOOTH,
				     .radius = 8.0f,
				     .strength = 1.0f,
				     .softness = 0.0f };

	*height_at(&land, 8, 8) = 10.0f;
	(void)voe_3d_landscape_brush(&land, &brush, 0.0f, 0.0f, 0.1f, arena);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 8, 8), 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 9, 8), 1.25f, 1e-5f);
}

// Strength 0.5 for 0.1 s moves the middle half way to a 2 m target.
static void flatten_moves_toward_the_target(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(64.0f, 16, arena);
	const voe_3d_brush brush = { .kind = VOE_3D_BRUSH_FLATTEN,
				     .radius = 8.0f,
				     .strength = 0.5f,
				     .softness = 0.5f,
				     .target = 2.0f };

	(void)voe_3d_landscape_brush(&land, &brush, 0.0f, 0.0f, 0.1f, arena);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 8, 8), 1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 10, 8), 0.0f, 0.0f);
}

// 8 cells make chunks of 2: chunk 5 is (1, 1), 9 vertices from (-4, -4) at
// uv 0.25, 8 triangles each facing +Y, on a sloped grid too.
static void chunk_has_its_quads_wound_up(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 8, arena);

	*height_at(&land, 3, 3) = 3.0f;
	const voe_3d_landscape_mesh mesh =
		voe_3d_landscape_chunk(&land, 5, arena);

	VOE_TEST_CHECK_INT(mesh.vertex_count, 9);
	VOE_TEST_CHECK_INT(mesh.index_count, 24);
	VOE_TEST_CHECK_FLOAT(mesh.vertices[0].position.x, -4.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(mesh.vertices[0].position.z, -4.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(mesh.vertices[0].uv.x, 0.25f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(mesh.vertices[4].position.y, 3.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(mesh.vertices[4].normal.y, 1.0f, 1e-6f);
	VOE_TEST_CHECK(mesh.vertices[3].normal.x < 0.0f);
	for (uint32_t i = 0; i < mesh.index_count; i += 3) {
		const voe_math_float3 a =
			mesh.vertices[mesh.indices[i]].position;
		const voe_math_float3 b =
			mesh.vertices[mesh.indices[i + 1]].position;
		const voe_math_float3 c =
			mesh.vertices[mesh.indices[i + 2]].position;
		const voe_math_float3 n = voe_math_float3_cross(
			voe_math_float3_sub(b, a), voe_math_float3_sub(c, a));

		VOE_TEST_CHECK(n.y > 0.0f);
	}
}

// 16 cells make chunks of 4: height 4 is chunk 0's last row and chunk 1's
// first, so it touches both; height 5 only chunk 1; the far edge chunk 3.
static void chunks_of_a_rect_on_an_edge_are_both(voe_base_arena *arena)
{
	const voe_assets_landscape land =
		voe_assets_landscape_flat(64.0f, 16, arena);
	const voe_3d_landscape_rect edge = voe_3d_landscape_chunks(
		&land, (voe_3d_landscape_rect){ .x0 = 4, .x1 = 5, .z0 = 0,
						.z1 = 1 });
	const voe_3d_landscape_rect inside = voe_3d_landscape_chunks(
		&land, (voe_3d_landscape_rect){ .x0 = 5, .x1 = 6, .z0 = 16,
						.z1 = 17 });
	const voe_3d_landscape_rect none = voe_3d_landscape_chunks(
		&land, (voe_3d_landscape_rect){ .x0 = 5, .x1 = 5, .z0 = 0,
						.z1 = 1 });

	VOE_TEST_CHECK_INT(edge.x0, 0);
	VOE_TEST_CHECK_INT(edge.x1, 2);
	VOE_TEST_CHECK_INT(edge.z0, 0);
	VOE_TEST_CHECK_INT(edge.z1, 1);
	VOE_TEST_CHECK_INT(inside.x0, 1);
	VOE_TEST_CHECK_INT(inside.x1, 2);
	VOE_TEST_CHECK_INT(inside.z0, 3);
	VOE_TEST_CHECK_INT(inside.z1, 4);
	VOE_TEST_CHECK(none.x0 >= none.x1);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);

	height_is_bilinear_between_four(arena);
	ray_meets_a_raised_cell(arena);
	ray_beside_the_grid_misses(arena);
	raise_lifts_the_middle_most_and_the_rim_not_at_all(arena);
	smooth_lowers_a_spike(arena);
	flatten_moves_toward_the_target(arena);
	chunk_has_its_quads_wound_up(arena);
	chunks_of_a_rect_on_an_edge_are_both(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
