// THE CLAIMS, on `.glb` files built here (model_glb.inc), none needing a
// graphics card:
//
// 1. A 64 × 32 sphere's levels made, then read against the same `.glb`: at
//    least 3 levels, each level's triangles at most three quarters of the one
//    before (level 0's being the sphere's), errors above nought and
//    non-decreasing.
// 2. That file read against the `.glb` with one byte changed, cut by 4 bytes,
//    and with its version changed: each false, stale.
// 3. The sphere with a second root "Ball_LOD1": its file reads `own`, one level.
// 4. The hash of "a" is FNV-1a 64's, 0xaf63dc4c8601ec8c.
// 5. Making levels of bytes that are no `.glb`: NULL, MALFORMED.
#include <3d/model_levels.h>

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
	const uint8_t *file =
		voe_3d_model_levels_make(arena, glb, glb_size, &size, &error);
	voe_3d_model_levels levels;
	uint64_t previous = SEGMENTS * (RINGS - 1) * 2;
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
}

static void own_levels(voe_base_arena *arena)
{
	size_t glb_size;
	const uint8_t *glb = sphere_glb(arena, 8, 16, true, &glb_size);
	voe_base_error error = VOE_BASE_OK;
	size_t size = 0;
	const uint8_t *file =
		voe_3d_model_levels_make(arena, glb, glb_size, &size, &error);
	voe_3d_model_levels levels;

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
						sizeof(text), &size,
						&error) == NULL);
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	sphere_levels(arena);
	own_levels(arena);
	VOE_TEST_CHECK(voe_3d_model_levels_hash((const uint8_t *)"a", 1) ==
		       0xaf63dc4c8601ec8cull);
	not_a_glb(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
