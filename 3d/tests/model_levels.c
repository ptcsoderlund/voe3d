// THE CLAIMS, on `.glb` files built here (model_glb.inc), none needing a
// graphics card:
//
// 1. A 64 × 32 sphere's levels made with the defaults, then read against the
//    same `.glb`: at least 3 levels, each level's triangles at most three
//    quarters of the one before (level 0's being the sphere's), errors above
//    nought and non-decreasing.
// 2. That file read against the `.glb` with one byte changed, cut by 4 bytes,
//    and with its version changed: each false, stale.
// 3. The sphere with a second root "Ball_LOD1": its file reads `own`, one level.
// 4. The hash of "a" is FNV-1a 64's, 0xaf63dc4c8601ec8c.
// 5. Making levels of bytes that are no `.glb`: NULL, MALFORMED.
// 6. The sphere made with count 2: at most 2 levels read; with count 1:
//    exactly 1.
// 7. The sphere made strong: level 1 has fewer triangles than made medium.
// 8. The defaults' file with its version field set to 1: read false, stale.
// 9. A view's level of a hand-filled entry (radius 1, bounds infinite, 0.2,
//    0.05) on the −z axis of a 60° projection, the distances from its P[1][1]:
//    0 with the eye inside, 0 at 2 m, 1 where s is 0.1, 2 where s is 0.01, and
//    below 2 there when scaled by 10.
#include <3d/model_levels.h>
#include <3d/projection.h>

#include <base/arena.h>
#include <base/error.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "model_glb.inc"

#define RINGS 32
#define SEGMENTS 64

static void sphere_levels(voe_base_arena *arena)
{
	size_t glb_size;
	const uint8_t *glb = sphere_glb(arena, RINGS, SEGMENTS, false, &glb_size);
	voe_base_error error = VOE_BASE_OK;
	size_t size = 0;
	const uint8_t *file = voe_3d_model_levels_make(
		arena, glb, glb_size, VOE_ASSETS_LEVELS_DEFAULT, &size, &error);
	voe_3d_model_levels levels = {};
	uint64_t previous = SEGMENTS * (RINGS - 1) * 2;
	const uint32_t version_1 = 1;
	uint8_t *changed;

	VOE_TEST_CHECK(file != NULL);
	if (file == NULL)
		return;
	VOE_TEST_CHECK(voe_3d_model_levels_read(arena, file, size, glb, glb_size,
						&levels));
	VOE_TEST_CHECK(!levels.own);
	VOE_TEST_CHECK_INT(levels.part_count, 1);
	VOE_TEST_CHECK(levels.level_count >= 3);
	for (uint32_t k = 1; k < levels.level_count; k++) {
		const uint64_t triangles = levels.index_counts[k][0] / 3;

		VOE_TEST_CHECK(triangles * 4 <= previous * 3);
		VOE_TEST_CHECK(levels.errors[k] > 0.0f);
		VOE_TEST_CHECK(levels.errors[k] >= levels.errors[k - 1]);
		previous = triangles;
	}

	// 2: stale three ways.
	changed = voe_base_arena_push(arena, glb_size);
	memcpy(changed, glb, glb_size);
	changed[glb_size / 2] ^= 1;
	VOE_TEST_CHECK(!voe_3d_model_levels_read(arena, file, size, changed,
						 glb_size, &levels));
	VOE_TEST_CHECK(!voe_3d_model_levels_read(arena, file, size - 4, glb,
						 glb_size, &levels));
	changed = voe_base_arena_push(arena, size);
	memcpy(changed, file, size);
	changed[4] ^= 2;
	VOE_TEST_CHECK(!voe_3d_model_levels_read(arena, changed, size, glb,
						 glb_size, &levels));

	// 8: a file the earlier simplifier made.
	memcpy(changed, file, size);
	memcpy(changed + 4, &version_1, sizeof(version_1));
	VOE_TEST_CHECK(!voe_3d_model_levels_read(arena, changed, size, glb,
						 glb_size, &levels));
}

// The sphere's levels made as `asked` and read back into `out`; false when
// either failed.
static bool sphere_made(voe_base_arena *arena, voe_assets_levels asked,
			voe_3d_model_levels *out)
{
	size_t glb_size;
	const uint8_t *glb = sphere_glb(arena, RINGS, SEGMENTS, false, &glb_size);
	voe_base_error error = VOE_BASE_OK;
	size_t size = 0;
	const uint8_t *file = voe_3d_model_levels_make(arena, glb, glb_size,
						       asked, &size, &error);

	return file != NULL && voe_3d_model_levels_read(arena, file, size, glb,
							glb_size, out);
}

static void levels_asked(voe_base_arena *arena)
{
	voe_3d_model_levels levels = {};
	voe_3d_model_levels medium = {};

	// 6
	VOE_TEST_CHECK(sphere_made(
		arena, (voe_assets_levels){ 2, VOE_ASSETS_LEVELS_MEDIUM },
		&levels));
	VOE_TEST_CHECK(levels.level_count <= 2);
	VOE_TEST_CHECK(sphere_made(
		arena, (voe_assets_levels){ 1, VOE_ASSETS_LEVELS_MEDIUM },
		&levels));
	VOE_TEST_CHECK_INT(levels.level_count, 1);

	// 7
	VOE_TEST_CHECK(sphere_made(
		arena, (voe_assets_levels){ 2, VOE_ASSETS_LEVELS_MEDIUM },
		&medium));
	VOE_TEST_CHECK(sphere_made(
		arena, (voe_assets_levels){ 2, VOE_ASSETS_LEVELS_STRONG },
		&levels));
	VOE_TEST_CHECK(medium.level_count == 2 && levels.level_count == 2);
	VOE_TEST_CHECK(levels.index_counts[1][0] < medium.index_counts[1][0]);
}

static void own_levels(voe_base_arena *arena)
{
	size_t glb_size;
	const uint8_t *glb = sphere_glb(arena, 8, 16, true, &glb_size);
	voe_base_error error = VOE_BASE_OK;
	size_t size = 0;
	const uint8_t *file = voe_3d_model_levels_make(
		arena, glb, glb_size, VOE_ASSETS_LEVELS_DEFAULT, &size, &error);
	voe_3d_model_levels levels = {};

	VOE_TEST_CHECK(file != NULL);
	if (file == NULL)
		return;
	VOE_TEST_CHECK(voe_3d_model_levels_read(arena, file, size, glb, glb_size,
						&levels));
	VOE_TEST_CHECK(levels.own);
	VOE_TEST_CHECK_INT(levels.level_count, 1);
}

static void not_a_glb(voe_base_arena *arena)
{
	static const char text[] = "not a model at all, only words";
	voe_base_error error = VOE_BASE_OK;
	size_t size = 0;

	VOE_TEST_CHECK(voe_3d_model_levels_make(arena, (const uint8_t *)text,
						sizeof(text),
						VOE_ASSETS_LEVELS_DEFAULT,
						&size, &error) == NULL);
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
}

static uint32_t level_at(const voe_3d_model_entry *model,
			 const voe_render_view *view, double distance,
			 float scale)
{
	const voe_scene_transform placed = {
		.position = { 0.0, 0.0, -distance },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { scale, scale, scale },
	};

	return voe_3d_model_level(model, &placed, (voe_math_double3){ 0 }, view);
}

static void view_levels(void)
{
	static voe_3d_model_entry model;
	const voe_scene_camera lens = {
		.fov_y = 1.0471976f, // 60°
		.near_plane = 0.1f,
		.far_plane = 1000.0f,
	};
	const voe_render_view view = {
		.view = voe_math_float4x4_identity(),
		.projection = voe_3d_projection(lens, 1.0f),
	};
	const double focal = view.projection.m[1][1];

	model.radius = 1.0f;
	model.level_count = 3;
	model.levels[0].bound = INFINITY;
	model.levels[1].bound = 0.2f;
	model.levels[2].bound = 0.05f;

	VOE_TEST_CHECK_INT(level_at(&model, &view, 0.5, 1.0f), 0);
	VOE_TEST_CHECK_INT(level_at(&model, &view, 2.0, 1.0f), 0);
	VOE_TEST_CHECK_INT(level_at(&model, &view, focal / 0.1, 1.0f), 1);
	VOE_TEST_CHECK_INT(level_at(&model, &view, focal / 0.01, 1.0f), 2);
	VOE_TEST_CHECK(level_at(&model, &view, focal / 0.01, 10.0f) < 2);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4 * 1024 * 1024);

	sphere_levels(arena);
	own_levels(arena);
	VOE_TEST_CHECK(voe_3d_model_levels_hash((const uint8_t *)"a", 1) ==
		       0xaf63dc4c8601ec8cull);
	not_a_glb(arena);
	levels_asked(arena);
	view_levels();
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
