// A tree grown by 3d/tree_grow.h from the species defaults, prepared in memory
// by voe_3d_model_prepare_grown (3d/model_prepare.h): it prepares with
// VOE_3D_TREE_LEVELS own levels, measured; their errors rise from level to
// level; and voe_3d_model_levels_bounds on them swaps each level in farther
// than the one before. Reads the prepared struct through the store's private
// header. Needs no graphics card.
#include "../src/models_store.h"

#include <3d/model_levels.h>
#include <3d/model_prepare.h>
#include <3d/tree_grow.h>
#include <assets/model.h>
#include <assets/tree.h>
#include <base/arena.h>
#include <base/error.h>

#include <testing/test.h>

#include <math.h>
#include <stdint.h>

#define ARENA (64 * 1024 * 1024)
// A scene camera's default lens, as the LODs panel shows metres for.
#define FOV_Y 1.0f

static const voe_3d_model_prepared *grown(voe_base_arena *arena)
{
	const voe_assets_tree values = voe_assets_tree_default();
	voe_3d_tree_skeleton skeleton;
	voe_assets_model *model = voe_base_arena_push(arena, sizeof(*model));
	voe_base_error error = { 0 };
	const voe_3d_model_prepared *prepared;

	voe_3d_tree_grow(arena, &values, 0, &skeleton);
	voe_3d_tree_mesh(arena, &skeleton, &values, 0, model);
	prepared = voe_3d_model_prepare_grown(arena, model, &error);
	VOE_TEST_CHECK(prepared != NULL);
	return prepared;
}

static void it_prepares_with_the_tree_levels(
	const voe_3d_model_prepared *prepared)
{
	VOE_TEST_CHECK(prepared->own);
	VOE_TEST_CHECK(prepared->measured);
	VOE_TEST_CHECK(!prepared->made);
	VOE_TEST_CHECK_INT(prepared->own_count, VOE_3D_TREE_LEVELS);
	VOE_TEST_CHECK_INT(prepared->read.level_count, VOE_3D_TREE_LEVELS);
	VOE_TEST_CHECK(prepared->radius > 0.0f);
}

static void the_errors_rise(const voe_3d_model_prepared *prepared)
{
	const float *errors = prepared->read.errors;

	VOE_TEST_CHECK(errors[0] == 0.0f);
	for (uint32_t k = 1; k < VOE_3D_TREE_LEVELS; k++) {
		VOE_TEST_CHECK(isfinite(errors[k]));
		VOE_TEST_CHECK(errors[k] > errors[k - 1]);
	}
}

static void the_bounds_rise(const voe_3d_model_prepared *prepared)
{
	float bounds[VOE_3D_MODEL_LEVELS];
	float before = 0.0f;

	voe_3d_model_levels_bounds(prepared->radius, prepared->read.errors,
				   prepared->read.swaps, prepared->own_count,
				   prepared->measured, bounds);
	VOE_TEST_CHECK(isinf(bounds[0]));
	for (uint32_t k = 1; k < VOE_3D_TREE_LEVELS; k++) {
		const float metres = voe_3d_model_level_metres(
			prepared->radius, bounds[k], FOV_Y);

		VOE_TEST_CHECK(bounds[k] > 0.0f && bounds[k] <= 1.0f);
		VOE_TEST_CHECK(bounds[k] < bounds[k - 1]);
		VOE_TEST_CHECK(metres > before);
		before = metres;
	}
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);
	const voe_3d_model_prepared *prepared = grown(arena);

	if (prepared != NULL) {
		it_prepares_with_the_tree_levels(prepared);
		the_errors_rise(prepared);
		the_bounds_rise(prepared);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
