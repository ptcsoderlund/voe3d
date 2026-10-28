// THE CLAIMS, on the sphere `.glb` of model_glb.inc given three 8 × 8
// pictures its material names as base colour, metal-roughness and normal, none
// needing a graphics card:
//
// 1. round_trip_keeps_the_bake: a 16 × 32 sphere prepared with a levels file
//    made at the defaults, and the same sphere with its own level, each
//    written and read back: every part's vertex and index bytes of level 0,
//    of every own level and of every made level the same, the sphere, the
//    flags, the own count, the level and part counts and the edge count too.
// 2. round_trip_keeps_every_picture: each picture, given as blocks of known
//    bytes over its full chain in its kind's format, reads back with
//    voe_assets_image_level_count(8, 8) levels, the colour and metal-roughness
//    ones BC7 and the normal one BC5 (0477), and those bytes.
// 3. kind_is_first_use: voe_3d_model_cooked_kind says COLOUR, DATA and NORMAL
//    for the base-colour, metal-roughness and normal pictures, and COLOUR for
//    a fourth no material names.
// 4. key_comes_back: the key written is the read's and the head's alone.
// 5. a_changed_version_is_refused: the copy with its version word raised by
//    one, or set to 1, the version before its pictures were blocks, reads
//    NULL, and its head gives no key.
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
#define PICTURES 3u

static const voe_3d_model_cooked_key KEY = { 11, 22 };

// The sphere, with its own level or a made levels file, read, given its
// pictures (0 base colour, 1 metal-roughness, 2 normal), prepared; NULL when
// any step fails.
static const voe_3d_model_prepared *sphere_prepared(voe_base_arena *arena,
						    bool own)
{
	voe_base_error error = VOE_BASE_OK;
	voe_assets_model *model = voe_base_arena_push(arena, sizeof(*model));
	voe_assets_material *materials;
	voe_assets_image *images =
		voe_base_arena_push(arena, PICTURES * sizeof(*images));
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
	materials[0].metallic_roughness_image = 1;
	materials[0].normal_image = 2;
	model->materials = materials;
	for (uint32_t p = 0; p < PICTURES; p++) {
		voe_assets_image *image = &images[p];

		*image = (voe_assets_image){
			.width = SIDE, .height = SIDE, .levels = 1,
			.pixels = voe_base_arena_push(arena, SIDE * SIDE * 4)
		};
		for (uint32_t i = 0; i < SIDE * SIDE * 4; i++)
			image->pixels[i] = (uint8_t)(i * 7 + p);
	}
	model->images = images;
	model->image_count = PICTURES;
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

// Byte `i` of picture `p`'s blocks as given to the write.
static uint8_t known_byte(uint32_t p, size_t i)
{
	return (uint8_t)(i * 5 + p * 11 + 3);
}

// One picture a model image, the blocks of its full chain in its kind's
// format, each byte known_byte's: what the caller of the write hands it.
static const voe_assets_image *blocks_of(voe_base_arena *arena,
					 const voe_assets_model *model)
{
	voe_assets_image *pictures = voe_base_arena_push(
		arena, (model->image_count + 1) * sizeof(*pictures));

	for (uint32_t p = 0; p < model->image_count; p++) {
		const voe_assets_image *image = &model->images[p];
		const voe_assets_image chain = {
			.width = image->width, .height = image->height,
			.levels = voe_assets_image_level_count(image->width,
							       image->height)
		};

		pictures[p] = voe_assets_image_blocks_new(
			arena, &chain, voe_3d_model_cooked_kind(model, p));
		for (size_t i = 0; i < voe_assets_image_size(&pictures[p]); i++)
			pictures[p].pixels[i] = known_byte(p, i);
	}
	return pictures;
}

// `prepared` written with KEY and its pictures' blocks.
static voe_assets_bytes write_copy(voe_base_arena *arena,
				   const voe_3d_model_prepared *prepared)
{
	return voe_3d_model_cooked_write(
		arena, prepared, blocks_of(arena, prepared->model), KEY);
}

// `prepared` written with KEY and read back; NULL when either fails.
static const voe_3d_model_prepared *
round_trip(voe_base_arena *arena, const voe_3d_model_prepared *prepared,
	   voe_3d_model_cooked_key *key)
{
	const voe_assets_bytes copy = write_copy(arena, prepared);

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

static void round_trip_keeps_every_picture(voe_base_arena *arena)
{
	static const voe_assets_image_format formats[PICTURES] = {
		VOE_ASSETS_IMAGE_BC7, VOE_ASSETS_IMAGE_BC7, VOE_ASSETS_IMAGE_BC5
	};
	const voe_3d_model_prepared *was = sphere_prepared(arena, false);
	voe_3d_model_cooked_key key;
	const voe_3d_model_prepared *is =
		was == NULL ? NULL : round_trip(arena, was, &key);

	VOE_TEST_CHECK(is != NULL && is->model->image_count == PICTURES);
	if (is == NULL || is->model->image_count != PICTURES)
		return;
	for (uint32_t p = 0; p < PICTURES; p++) {
		const voe_assets_image *image = &is->model->images[p];
		bool kept = true;

		VOE_TEST_CHECK_INT(image->levels,
				   voe_assets_image_level_count(SIDE, SIDE));
		VOE_TEST_CHECK_INT(image->format, formats[p]);
		for (size_t i = 0; i < voe_assets_image_size(image); i++)
			kept = kept && image->pixels[i] == known_byte(p, i);
		VOE_TEST_CHECK(kept);
	}
	VOE_TEST_CHECK_INT(is->model->materials[0].base_colour_image, 0);
	VOE_TEST_CHECK_INT(is->model->materials[0].normal_image, 2);
}

// Base colour COLOUR, normal NORMAL, metal-roughness DATA, and a fourth
// picture no material names COLOUR.
static void kind_is_first_use(voe_base_arena *arena)
{
	const voe_3d_model_prepared *was = sphere_prepared(arena, false);
	voe_assets_model four;

	VOE_TEST_CHECK(was != NULL);
	if (was == NULL)
		return;
	four = *was->model;
	four.image_count = PICTURES + 1;
	VOE_TEST_CHECK_INT(voe_3d_model_cooked_kind(&four, 0),
			   VOE_ASSETS_IMAGE_COLOUR);
	VOE_TEST_CHECK_INT(voe_3d_model_cooked_kind(&four, 1),
			   VOE_ASSETS_IMAGE_DATA);
	VOE_TEST_CHECK_INT(voe_3d_model_cooked_kind(&four, 2),
			   VOE_ASSETS_IMAGE_NORMAL);
	VOE_TEST_CHECK_INT(voe_3d_model_cooked_kind(&four, 3),
			   VOE_ASSETS_IMAGE_COLOUR);
}

static void key_comes_back(voe_base_arena *arena)
{
	const voe_3d_model_prepared *was = sphere_prepared(arena, false);
	voe_3d_model_cooked_key read = { 0 }, head = { 0 };
	voe_assets_bytes copy = { 0 };

	VOE_TEST_CHECK(was != NULL);
	if (was == NULL)
		return;
	copy = write_copy(arena, was);
	VOE_TEST_CHECK(voe_3d_model_cooked_read(arena, copy.bytes, copy.count,
						&read) != NULL);
	VOE_TEST_CHECK(voe_3d_model_cooked_key_of(
		copy.bytes, VOE_3D_MODEL_COOKED_HEAD, &head));
	VOE_TEST_CHECK(read.glb == KEY.glb && read.parts == KEY.parts);
	VOE_TEST_CHECK(head.glb == KEY.glb && head.parts == KEY.parts);
}

// `old`: the version word set to 1, the layout before 0477; else raised by one.
static void a_changed_version_is_refused(voe_base_arena *arena, bool old)
{
	const voe_3d_model_prepared *was = sphere_prepared(arena, false);
	voe_3d_model_cooked_key key = { 0 };
	voe_assets_bytes copy = { 0 };
	uint32_t version;

	VOE_TEST_CHECK(was != NULL);
	if (was == NULL)
		return;
	copy = write_copy(arena, was);
	memcpy(&version, copy.bytes + 4, sizeof(version));
	version = old ? 1u : version + 1;
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
	round_trip_keeps_every_picture(arena);
	kind_is_first_use(arena);
	key_comes_back(arena);
	a_changed_version_is_refused(arena, false);
	a_changed_version_is_refused(arena, true);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
