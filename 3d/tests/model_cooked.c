// THE CLAIMS, on the sphere `.glb` of model_glb.inc given one 8 × 8 picture
// its material's base colour names, none needing a graphics card:
//
// 1. round_trip_keeps_the_bake: a 16 × 32 sphere prepared with a levels file
//    made at the defaults, and the same sphere with its own level, each
//    written and read back: every part's vertex and index bytes of level 0,
//    of every own level and of every made level the same, the sphere, the
//    flags, the own count, the level and part counts and the edge count too.
// 2. round_trip_mips_every_picture: the picture read back has
//    voe_assets_image_level_count(8, 8) levels.
// 3. key_comes_back: the key written is the read's and the head's alone.
// 4. a_changed_version_is_refused: the copy with its version word raised by
//    one reads NULL, and its head gives no key.
#include "../src/models_store.h"

#include <3d/model_cooked.h>
#include <3d/model_levels.h>
#include <3d/model_prepare.h>

#include <assets/image.h>
#include <assets/model.h>
#include <assets/parts.h>
#include <base/arena.h>
#include <base/error.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "model_glb.inc"

#define SIDE 8u

static const voe_3d_model_cooked_key KEY = { 11, 22 };

// The sphere, with its own level or a made levels file, read, given its
// picture, prepared; NULL when any step fails.
static const voe_3d_model_prepared *sphere_prepared(voe_base_arena *arena,
						    bool own)
{
	voe_base_error error = VOE_BASE_OK;
	voe_assets_model *model = voe_base_arena_push(arena, sizeof(*model));
	voe_assets_material *materials;
	voe_assets_image *image = voe_base_arena_push(arena, sizeof(*image));
	size_t size, levels_size = 0;
	const uint8_t *glb = sphere_glb(arena, 16, 32, own, &size);
	const uint8_t *levels = NULL;

	if (!voe_assets_model_read_glb(glb, size, arena, model, &error) ||
	    model->material_count == 0)
		return NULL;
	materials = voe_base_arena_push(
		arena, model->material_count * sizeof(*materials));
	memcpy(materials, model->materials,
	       model->material_count * sizeof(*materials));
	materials[0].base_colour_image = 0;
	model->materials = materials;
	*image = (voe_assets_image){ .width = SIDE, .height = SIDE, .levels = 1,
				     .pixels = voe_base_arena_push(
					     arena, SIDE * SIDE * 4) };
	for (uint32_t i = 0; i < SIDE * SIDE * 4; i++)
		image->pixels[i] = (uint8_t)(i * 7);
	model->images = image;
	model->image_count = 1;
	if (!own) {
		levels = voe_3d_model_levels_make(arena, glb, size,
						  VOE_ASSETS_LEVELS_DEFAULT,
						  &levels_size, &error);
		if (levels == NULL)
			return NULL;
	}
	return voe_3d_model_prepare(arena, model, glb, size, levels,
				    levels_size, &error);
}

// `prepared` written with KEY and read back; NULL when either fails.
static const voe_3d_model_prepared *
round_trip(voe_base_arena *arena, const voe_3d_model_prepared *prepared,
	   voe_3d_model_cooked_key *key)
{
	const voe_assets_bytes copy =
		voe_3d_model_cooked_write(arena, prepared, KEY);

	return voe_3d_model_cooked_read(arena, copy.bytes, copy.count, key);
}

static bool same(const void *a, const void *b, size_t size)
{
	return size == 0 || memcmp(a, b, size) == 0;
}

static void check_bake(const voe_3d_model_bake *was,
		       const voe_3d_model_bake *is)
{
	VOE_TEST_CHECK_INT(is->part_count, was->part_count);
	VOE_TEST_CHECK_INT(is->vertex_count, was->vertex_count);
	VOE_TEST_CHECK_INT(is->index_count, was->index_count);
	for (uint32_t p = 0; p < was->part_count && p < is->part_count; p++) {
		const voe_3d_model_bake_part *a = &was->parts[p];
		const voe_3d_model_bake_part *b = &is->parts[p];

		VOE_TEST_CHECK(same(a, b, sizeof(*a)));
		VOE_TEST_CHECK(same(was->vertices + a->first_vertex,
				    is->vertices + b->first_vertex,
				    a->vertex_count * sizeof(*was->vertices)));
		VOE_TEST_CHECK(same(was->indices + a->first_index,
				    is->indices + b->first_index,
				    a->index_count * sizeof(*was->indices)));
	}
}

static void check_levels(const voe_3d_model_levels *was,
			 const voe_3d_model_levels *is)
{
	VOE_TEST_CHECK_INT(is->own, was->own);
	VOE_TEST_CHECK_INT(is->level_count, was->level_count);
	VOE_TEST_CHECK_INT(is->part_count, was->part_count);
	VOE_TEST_CHECK(same(was->errors, is->errors, sizeof(was->errors)));
	VOE_TEST_CHECK(same(was->swaps, is->swaps, sizeof(was->swaps)));
	for (uint32_t k = 1; k < was->level_count; k++) {
		for (uint32_t p = 0; p < was->part_count; p++) {
			const uint32_t count = was->index_counts[k][p];

			VOE_TEST_CHECK_INT(is->index_counts[k][p], count);
			VOE_TEST_CHECK(same(was->indices[k][p],
					    is->indices[k][p],
					    count * sizeof(uint32_t)));
		}
	}
}

static void round_trip_keeps_the_bake(voe_base_arena *arena, bool own)
{
	const voe_3d_model_prepared *was = sphere_prepared(arena, own);
	voe_3d_model_cooked_key key;
	const voe_3d_model_prepared *is =
		was == NULL ? NULL : round_trip(arena, was, &key);

	VOE_TEST_CHECK(was != NULL && is != NULL);
	if (was == NULL || is == NULL)
		return;
	VOE_TEST_CHECK(was->own == own && was->bake.part_count > 0);
	VOE_TEST_CHECK(own ? was->own_count > 1 : was->made);
	check_bake(&was->bake, &is->bake);
	VOE_TEST_CHECK(same(&was->centre, &is->centre, sizeof(was->centre)));
	VOE_TEST_CHECK(was->radius == is->radius);
	VOE_TEST_CHECK(is->own == was->own && is->measured == was->measured &&
		       is->made == was->made);
	VOE_TEST_CHECK_INT(is->own_count, was->own_count);
	for (uint32_t k = 1; k < was->own_count && k < is->own_count; k++)
		check_bake(&was->own_levels[k], &is->own_levels[k]);
	if (was->made || was->measured)
		check_levels(&was->read, &is->read);
	VOE_TEST_CHECK(was->shape.edge_count > 0);
	VOE_TEST_CHECK_INT(is->shape.edge_count, was->shape.edge_count);
}

static void round_trip_mips_every_picture(voe_base_arena *arena)
{
	const voe_3d_model_prepared *was = sphere_prepared(arena, false);
	voe_3d_model_cooked_key key;
	const voe_3d_model_prepared *is =
		was == NULL ? NULL : round_trip(arena, was, &key);

	VOE_TEST_CHECK(is != NULL && is->model->image_count == 1);
	if (is == NULL || is->model->image_count != 1)
		return;
	VOE_TEST_CHECK_INT(is->model->images[0].levels,
			   voe_assets_image_level_count(SIDE, SIDE));
	VOE_TEST_CHECK_INT(is->model->materials[0].base_colour_image, 0);
}

static void key_comes_back(voe_base_arena *arena)
{
	const voe_3d_model_prepared *was = sphere_prepared(arena, false);
	voe_3d_model_cooked_key read = { 0 }, head = { 0 };
	voe_assets_bytes copy = { 0 };

	VOE_TEST_CHECK(was != NULL);
	if (was == NULL)
		return;
	copy = voe_3d_model_cooked_write(arena, was, KEY);
	VOE_TEST_CHECK(voe_3d_model_cooked_read(arena, copy.bytes, copy.count,
						&read) != NULL);
	VOE_TEST_CHECK(voe_3d_model_cooked_key_of(
		copy.bytes, VOE_3D_MODEL_COOKED_HEAD, &head));
	VOE_TEST_CHECK(read.glb == KEY.glb && read.parts == KEY.parts);
	VOE_TEST_CHECK(head.glb == KEY.glb && head.parts == KEY.parts);
}

static void a_changed_version_is_refused(voe_base_arena *arena)
{
	const voe_3d_model_prepared *was = sphere_prepared(arena, false);
	voe_3d_model_cooked_key key = { 0 };
	voe_assets_bytes copy = { 0 };
	uint32_t version;

	VOE_TEST_CHECK(was != NULL);
	if (was == NULL)
		return;
	copy = voe_3d_model_cooked_write(arena, was, KEY);
	memcpy(&version, copy.bytes + 4, sizeof(version));
	version++;
	memcpy(copy.bytes + 4, &version, sizeof(version));
	VOE_TEST_CHECK(voe_3d_model_cooked_read(arena, copy.bytes, copy.count,
						&key) == NULL);
	VOE_TEST_CHECK(!voe_3d_model_cooked_key_of(copy.bytes, copy.count,
						   &key));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(16 * 1024 * 1024);

	round_trip_keeps_the_bake(arena, false);
	round_trip_keeps_the_bake(arena, true);
	round_trip_mips_every_picture(arena);
	key_comes_back(arena);
	a_changed_version_is_refused(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
