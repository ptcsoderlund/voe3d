// THE CLAIMS, on `.glb` files built here (model_glb.inc), none needing a
// graphics card:
//
// 1. A 64 × 32 sphere's levels made with the defaults, then read against the
//    same `.glb`: at least 3 levels, each level's triangles fewer than the one
//    before (level 0's being the sphere's), errors above nought and
//    non-decreasing.
// 2. That file read against the `.glb` with one byte changed, cut by 4 bytes,
//    and with its version changed: each false, stale.
// 3. The sphere with a second root "Ball_LOD1": its file reads `own`, 2 levels,
//    `errors[1]` finite and above nought.
// 4. The hash of "a" is FNV-1a 64's, 0xaf63dc4c8601ec8c.
// 5. Making levels of bytes that are no `.glb`: NULL, MALFORMED.
// 6. The sphere made with count 2: at most 2 levels read; with count 1:
//    exactly 1.
// 7. The sphere made with shares 80 and 10 for levels 1 and 2, count 3: level
//    1's triangles within 10 % of 80 % of level 0's, level 2's at most 15 % of
//    level 0's.
// 8. The defaults' file with its version field set to 1, to 2, to 3 and to 4:
//    read false, stale.
// 9. A view's level of a hand-filled entry (radius 1, bounds infinite, 0.2,
//    0.05) on the −z axis of a 60° projection, the distances from its P[1][1]:
//    0 with the eye inside, 0 at 2 m, 1 where s is 0.1, 2 where s is 0.01, and
//    below 2 there when scaled by 10.
// 10. A view's choice on 9's entry, in level 1's band from 0.2 down to 0.16,
//     where s is about 0.19: level 1 showing 0.25 of its pixels (within 0.01).
// 11. Where s is about 0.15, below the band: level 1, whole.
// 12. With the third bound 0.19, narrowing the band, s about 0.195: level 1
//     showing 0.5 (within 0.05).
// 13. The eye inside the sphere: level 0, whole.
// 14. At each of 9's distances, voe_3d_model_level is the choice's level.
// 15. The sphere made with swaps 1, 2, 0.5, 1, 1, 1: read gives them back; at
//     the defaults, all 1.
// 16. 1's sphere, made with VOE_ASSETS_LEVELS_DEFAULT: `level_count` above 4.
// 17. The defaults' file with swap 3 on level 2 set by _swaps_set: still read,
//     `swaps[2]` 3, its indices and errors unchanged.
// 18. _triangles of 3's sphere: 2, `own`, each above nought and level 1's
//     below level 0's; `fresh` false with NULL levels, true with its file.
// 19. _triangles of 1's sphere with the defaults' file: its `level_count`,
//     level 0 the sphere's triangles, level k its index counts' sum ÷ 3, not
//     `own`; with NULL levels, 1.
// 20. _triangles of bytes that are no `.glb`: 0.
// 21. The sphere's file made with a pool of 3 is byte for byte the file made
//     with NULL, with the defaults and with six levels of other shares.
#include <3d/model_levels.h>
#include <3d/projection.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/workers.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "model_glb.inc"

#define RINGS 32
#define SEGMENTS 64
#define TRIANGLES (SEGMENTS * (RINGS - 1) * 2)

// 17 on the defaults' `file`, a copy of it patched.
static void swaps_patched(voe_base_arena *arena, const uint8_t *file,
			  size_t size, const uint8_t *glb, size_t glb_size)
{
	float swaps[VOE_3D_MODEL_LEVELS] = { 1, 1, 3, 1, 1, 1 };
	uint8_t *patched = voe_base_arena_push(arena, size);
	voe_3d_model_levels before = {};
	voe_3d_model_levels after = {};

	memcpy(patched, file, size);
	voe_3d_model_levels_swaps_set(patched, size, swaps);
	VOE_TEST_CHECK(voe_3d_model_levels_read(arena, file, size, glb, glb_size,
						&before));
	VOE_TEST_CHECK(voe_3d_model_levels_read(arena, patched, size, glb,
						glb_size, &after));
	VOE_TEST_CHECK(after.swaps[2] == 3.0f);
	VOE_TEST_CHECK_INT(after.level_count, before.level_count);
	VOE_TEST_CHECK_INT(after.part_count, before.part_count);
	for (uint32_t k = 1; k < before.level_count; k++) {
		const uint32_t count = before.index_counts[k][0];

		VOE_TEST_CHECK(after.errors[k] == before.errors[k]);
		VOE_TEST_CHECK_INT(after.index_counts[k][0], count);
		VOE_TEST_CHECK(after.index_counts[k][0] != count ||
			       memcmp(after.indices[k][0], before.indices[k][0],
				      (size_t)count * sizeof(uint32_t)) == 0);
	}
}

// 19 on the defaults' `file` and its read `levels`.
static void made_triangles(voe_base_arena *arena, const uint8_t *glb,
			   size_t glb_size, const uint8_t *file, size_t size,
			   const voe_3d_model_levels *levels)
{
	uint32_t triangles[VOE_3D_MODEL_LEVELS] = {};
	bool own = true;
	bool fresh = false;
	const uint32_t count = voe_3d_model_levels_triangles(
		arena, glb, glb_size, file, size, triangles, &own, &fresh);

	VOE_TEST_CHECK_INT(count, levels->level_count);
	VOE_TEST_CHECK(!own);
	VOE_TEST_CHECK(fresh);
	VOE_TEST_CHECK_INT(triangles[0], TRIANGLES);
	for (uint32_t k = 1; k < count && k < VOE_3D_MODEL_LEVELS; k++)
		VOE_TEST_CHECK_INT(triangles[k], levels->index_counts[k][0] / 3);
	VOE_TEST_CHECK_INT(voe_3d_model_levels_triangles(arena, glb, glb_size,
							 NULL, 0, triangles,
							 &own, &fresh),
			   1);
	VOE_TEST_CHECK(!fresh);
}

static void sphere_levels(voe_base_arena *arena)
{
	size_t glb_size;
	const uint8_t *glb = sphere_glb(arena, RINGS, SEGMENTS, false, &glb_size);
	voe_base_error error = VOE_BASE_OK;
	size_t size = 0;
	const uint8_t *file =
		voe_3d_model_levels_make(arena, NULL, glb, glb_size,
					 VOE_ASSETS_LEVELS_DEFAULT, &size, &error);
	voe_3d_model_levels levels = {};
	uint64_t previous = TRIANGLES;
	uint8_t *changed;

	VOE_TEST_CHECK(file != NULL);
	if (file == NULL)
		return;
	VOE_TEST_CHECK(voe_3d_model_levels_read(arena, file, size, glb, glb_size,
						&levels));
	VOE_TEST_CHECK(!levels.own);
	VOE_TEST_CHECK_INT(levels.part_count, 1);
	VOE_TEST_CHECK(levels.level_count >= 3);
	// 16
	VOE_TEST_CHECK(levels.level_count > 4);
	for (uint32_t k = 1; k < levels.level_count; k++) {
		const uint64_t triangles = levels.index_counts[k][0] / 3;

		VOE_TEST_CHECK(triangles < previous);
		VOE_TEST_CHECK(levels.errors[k] > 0.0f);
		VOE_TEST_CHECK(levels.errors[k] >= levels.errors[k - 1]);
		previous = triangles;
	}
	made_triangles(arena, glb, glb_size, file, size, &levels);

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

	// 8: files the earlier simplifier made, before and after the strength,
	// those made when the default was four levels, and those with one swap.
	memcpy(changed, file, size);
	for (uint32_t version = 1; version <= 4; version++) {
		memcpy(changed + 4, &version, sizeof(version));
		VOE_TEST_CHECK(!voe_3d_model_levels_read(arena, changed, size,
							 glb, glb_size, &levels));
	}

	swaps_patched(arena, file, size, glb, glb_size);
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
	const uint8_t *file = voe_3d_model_levels_make(arena, NULL, glb, glb_size,
						       asked, &size, &error);

	return file != NULL && voe_3d_model_levels_read(arena, file, size, glb,
							glb_size, out);
}

static void levels_asked(voe_base_arena *arena)
{
	const float swaps[VOE_3D_MODEL_LEVELS] = { 1, 2, 0.5f, 1, 1, 1 };
	voe_3d_model_levels levels = {};
	voe_assets_levels asked = VOE_ASSETS_LEVELS_DEFAULT;

	// 6
	asked.count = 2;
	VOE_TEST_CHECK(sphere_made(arena, asked, &levels));
	VOE_TEST_CHECK(levels.level_count <= 2);
	asked.count = 1;
	VOE_TEST_CHECK(sphere_made(arena, asked, &levels));
	VOE_TEST_CHECK_INT(levels.level_count, 1);

	// 7
	asked = VOE_ASSETS_LEVELS_DEFAULT;
	asked.count = 3;
	asked.shares[1] = 80;
	asked.shares[2] = 10;
	VOE_TEST_CHECK(sphere_made(arena, asked, &levels));
	VOE_TEST_CHECK_INT(levels.level_count, 3);
	VOE_TEST_CHECK(fabs(levels.index_counts[1][0] / 3 - 0.8 * TRIANGLES) <=
		       0.1 * 0.8 * TRIANGLES);
	VOE_TEST_CHECK(levels.index_counts[2][0] / 3 <= 0.15 * TRIANGLES);

	// 15
	asked = VOE_ASSETS_LEVELS_DEFAULT;
	memcpy(asked.swaps, swaps, sizeof(swaps));
	VOE_TEST_CHECK(sphere_made(arena, asked, &levels));
	for (uint32_t k = 0; k < VOE_3D_MODEL_LEVELS; k++)
		VOE_TEST_CHECK(levels.swaps[k] == swaps[k]);
	VOE_TEST_CHECK(sphere_made(arena, VOE_ASSETS_LEVELS_DEFAULT, &levels));
	for (uint32_t k = 0; k < VOE_3D_MODEL_LEVELS; k++)
		VOE_TEST_CHECK(levels.swaps[k] == 1.0f);
}

// 21 as `asked`.
static void pooled_same(voe_base_arena *arena, voe_base_workers *workers,
			voe_assets_levels asked)
{
	size_t glb_size;
	const uint8_t *glb = sphere_glb(arena, RINGS, SEGMENTS, false, &glb_size);
	voe_base_error error = VOE_BASE_OK;
	size_t alone_size = 0;
	size_t pooled_size = 0;
	const uint8_t *alone = voe_3d_model_levels_make(
		arena, NULL, glb, glb_size, asked, &alone_size, &error);
	const uint8_t *pooled = voe_3d_model_levels_make(
		arena, workers, glb, glb_size, asked, &pooled_size, &error);

	VOE_TEST_CHECK(alone != NULL && pooled != NULL);
	VOE_TEST_CHECK_INT(pooled_size, alone_size);
	VOE_TEST_CHECK(alone != NULL && pooled != NULL &&
		       pooled_size == alone_size &&
		       memcmp(pooled, alone, alone_size) == 0);
}

static void pooled_levels(voe_base_arena *arena)
{
	voe_base_workers *workers = voe_base_workers_new(3);
	voe_assets_levels six = VOE_ASSETS_LEVELS_DEFAULT;
	const float shares[VOE_ASSETS_LEVELS] = { 100, 80, 40, 20, 10, 5 };

	six.count = 6;
	memcpy(six.shares, shares, sizeof(shares));
	pooled_same(arena, workers, VOE_ASSETS_LEVELS_DEFAULT);
	pooled_same(arena, workers, six);
	voe_base_workers_destroy(workers);
}

static void own_levels(voe_base_arena *arena)
{
	size_t glb_size;
	const uint8_t *glb = sphere_glb(arena, 8, 16, true, &glb_size);
	voe_base_error error = VOE_BASE_OK;
	size_t size = 0;
	const uint8_t *file =
		voe_3d_model_levels_make(arena, NULL, glb, glb_size,
					 VOE_ASSETS_LEVELS_DEFAULT, &size, &error);
	voe_3d_model_levels levels = {};
	uint32_t triangles[VOE_3D_MODEL_LEVELS] = {};
	bool own = false;
	bool fresh = true;

	VOE_TEST_CHECK(file != NULL);
	if (file == NULL)
		return;
	VOE_TEST_CHECK(voe_3d_model_levels_read(arena, file, size, glb, glb_size,
						&levels));
	VOE_TEST_CHECK(levels.own);
	VOE_TEST_CHECK_INT(levels.level_count, 2);
	VOE_TEST_CHECK(isfinite(levels.errors[1]) && levels.errors[1] > 0.0f);

	// 18
	VOE_TEST_CHECK_INT(voe_3d_model_levels_triangles(arena, glb, glb_size,
							 NULL, 0, triangles,
							 &own, &fresh),
			   2);
	VOE_TEST_CHECK(own);
	VOE_TEST_CHECK(!fresh);
	VOE_TEST_CHECK(triangles[1] > 0 && triangles[1] < triangles[0]);
	VOE_TEST_CHECK_INT(voe_3d_model_levels_triangles(arena, glb, glb_size,
							 file, size, triangles,
							 &own, &fresh),
			   2);
	VOE_TEST_CHECK(own);
	VOE_TEST_CHECK(fresh);
}

static void not_a_glb(voe_base_arena *arena)
{
	static const char text[] = "not a model at all, only words";
	voe_base_error error = VOE_BASE_OK;
	size_t size = 0;
	uint32_t triangles[VOE_3D_MODEL_LEVELS] = {};
	bool own = false;
	bool fresh = false;

	// 20
	VOE_TEST_CHECK_INT(voe_3d_model_levels_triangles(
				   arena, (const uint8_t *)text, sizeof(text),
				   NULL, 0, triangles, &own, &fresh),
			   0);
	VOE_TEST_CHECK(voe_3d_model_levels_make(arena, NULL,
						(const uint8_t *)text,
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

static voe_3d_model_level_choice choice_at(const voe_3d_model_entry *model,
					   const voe_render_view *view,
					   double distance, float scale)
{
	const voe_scene_transform placed = {
		.position = { 0.0, 0.0, -distance },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { scale, scale, scale },
	};

	return voe_3d_model_level_choose(model, &placed, (voe_math_double3){ 0 },
					 view);
}

// 10 to 14 on view_levels' entry and view; its third bound is put back.
static void view_shares(voe_3d_model_entry *model, const voe_render_view *view)
{
	const double focal = view->projection.m[1][1];
	const double distances[] = { 0.5, 2.0, focal / 0.1, focal / 0.01 };
	voe_3d_model_level_choice choice;

	// 10, 11, 13
	choice = choice_at(model, view, focal / 0.19, 1.0f);
	VOE_TEST_CHECK_INT(choice.level, 1);
	VOE_TEST_CHECK(fabsf(choice.shown - 0.25f) < 0.01f);
	choice = choice_at(model, view, focal / 0.15, 1.0f);
	VOE_TEST_CHECK_INT(choice.level, 1);
	VOE_TEST_CHECK(choice.shown == 1.0f);
	choice = choice_at(model, view, 0.5, 1.0f);
	VOE_TEST_CHECK_INT(choice.level, 0);
	VOE_TEST_CHECK(choice.shown == 1.0f);
	// 14
	for (size_t i = 0; i < sizeof(distances) / sizeof(distances[0]); i++)
		VOE_TEST_CHECK_INT(level_at(model, view, distances[i], 1.0f),
				   choice_at(model, view, distances[i], 1.0f).level);
	VOE_TEST_CHECK_INT(level_at(model, view, focal / 0.01, 10.0f),
			   choice_at(model, view, focal / 0.01, 10.0f).level);

	// 12
	model->levels[2].bound = 0.19f;
	choice = choice_at(model, view, focal / 0.195, 1.0f);
	VOE_TEST_CHECK_INT(choice.level, 1);
	VOE_TEST_CHECK(fabsf(choice.shown - 0.5f) < 0.05f);
	model->levels[2].bound = 0.05f;
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
	view_shares(&model, &view);
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
	pooled_levels(arena);
	view_levels();
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
