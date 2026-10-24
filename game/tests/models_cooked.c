// The model loader's cooked copies against real files: the one-triangle
// `.glb` of glb_build.h written into a scratch folder in the working
// directory (the build tree), and a game world whose model row names it. Each
// case loads into a store of its own.
//
// update_writes_the_copy: an update leaves `Cache/models/good.glb.model`,
// keyed by the `.glb`'s stamp and 0 for no `.parts`.
// a_fresh_copy_is_read_not_written_again: a second update loads, the copy's
// stamp unchanged.
// a_changed_glb_writes_it_again: the `.glb` rewritten a size longer, an update
// writes the copy keyed by its new stamp.
// watch_reloads_and_writes_no_copy: the copy gone and the `.glb` changed, the
// watch reloads it at its new stamp and no copy is there.
// update_cooked_loads_a_copy_whose_glb_is_garbage: a copy written, then the
// `.glb` made bytes no `.glb` has, the game's update loads it from the copy,
// which keeps its old key.
// prepare_writes_the_copy: with the levels file fresh and the copy gone, a
// prepare makes it, keyed by the stamp; a second prepare makes nothing.
//
// The world's shape also wears a material whose colour, normal and ORM maps
// are one 16×16 PNG:
// materials_write_the_map_copy: the materials call leaves
// `Cache/pictures/pic.png.<colour|normal|data>`, each a full chain keyed by
// the PNG's stamp, BC7 but the normal's BC5 (0477).
// material_load_writes_no_copy: the one-material load, the copy gone, loads
// and writes none.
// materials_cooked_loads_a_copy_whose_png_is_garbage: a copy written, then
// the PNG made garbage, the game's materials call loads from the copy, kept.
//
// The editor's check before Play and Ship, on the `.glb` and the material:
// stale_counts_missing_copies: all four copies gone, 4. cook_makes_them_and_
// stale_is_then_0: all written at their stamps, compressed as above, the
// progress 4/4. an_old_map_copy_counts_stale: the colour copy's version set
// back to 1, 1; cooked again, 0.
// stale_counts_a_changed_glb: rewritten another size, 1.
// cook_stops_when_asked: stop set, false and the copy keeps its old key.
//
// The files and the folders are removed at the end, pass or fail. It skips
// when there is no graphics card, because a load uploads.
#include "glb_build.h"

#include <game/models.h>
#include <game/world.h>

#include <3d/model_cooked.h>
#include <3d/model_levels.h>
#include <3d/models.h>
#include <3d/shape_component.h>

#include <assets/image.h>

#include <base/arena.h>
#include <base/error.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FOLDER "game_models_cooked_test"
#define GOOD "good.glb"
#define GOOD_ON_DISK FOLDER "/" GOOD
#define CACHE_ON_DISK FOLDER "/Cache"
#define MODELS_FOLDER_ON_DISK CACHE_ON_DISK "/models"
#define COPY_ON_DISK MODELS_FOLDER_ON_DISK "/" GOOD ".model"
#define LEVELS_FOLDER_ON_DISK CACHE_ON_DISK "/levels"
#define LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" GOOD ".levels"
#define PICTURE "pic.png"
#define PICTURE_SIDE 16
#define PICTURE_ON_DISK FOLDER "/" PICTURE
#define PICTURES_FOLDER_ON_DISK CACHE_ON_DISK "/pictures"
#define MAP_COPY_ON_DISK PICTURES_FOLDER_ON_DISK "/" PICTURE ".colour"
#define NORMAL_COPY_ON_DISK PICTURES_FOLDER_ON_DISK "/" PICTURE ".normal"
#define DATA_COPY_ON_DISK PICTURES_FOLDER_ON_DISK "/" PICTURE ".data"
#define MATERIAL "pic.material"

// A white material wearing the PNG as its colour, normal and ORM maps, and no
// model's parts.
static const voe_game_materials MATERIALS = {
	(const voe_game_material[]){ { MATERIAL,
				       { .colour = { 1.0f, 1.0f, 1.0f },
					 .roughness = 0.5f,
					 .repeat = 1.0f,
					 .colormap = PICTURE,
					 .normalmap = PICTURE,
					 .ormmap = PICTURE } } },
	1
};
static const voe_game_parts NO_PARTS = { NULL, 0 };

// Room for the triangle twice over a watch's reload, with its levels, and the
// shadings of its part and a blended twin, each twice.
static const voe_render_capacities CAPACITIES = {
	.vertices = 64,
	.indices = 64,
	.geometries = VOE_3D_MODEL_LEVELS * 2,
	.objects = 1,
	.shadings = 16,
	.passes = 1,
};

// The one-triangle `.glb` written, its JSON `pad` spaces longer when not 0,
// so its size and stamp differ; its stamp returned.
static uint64_t glb_write(uint32_t pad)
{
	static char text[sizeof(JSON) + 16];
	uint8_t glb[512];
	uint32_t length;
	uint64_t stamp = 0;

	VOE_TEST_CHECK(pad < 16);
	snprintf(text, sizeof(text), "%s%*s", JSON, (int)pad, "");
	length = glb_pack(glb, sizeof(glb), text, TRIANGLE,
			  (uint32_t)sizeof(TRIANGLE));
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, glb, length, NULL));
	VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &stamp));
	return stamp;
}

// The copy's key from its head; zeroed when there is none or it will not read.
static voe_3d_model_cooked_key copy_key(void)
{
	uint8_t head[VOE_3D_MODEL_COOKED_HEAD];
	voe_3d_model_cooked_key key = { 0 };
	size_t count = 0;

	if (!voe_platform_file_read_head(COPY_ON_DISK, head, sizeof(head),
					 &count, NULL) ||
	    !voe_3d_model_cooked_key_of(head, count, &key))
		return (voe_3d_model_cooked_key){ 0 };
	return key;
}

// The copy's stamp, 0 when there is none.
static uint64_t copy_stamp(void)
{
	uint64_t stamp = 0;

	return voe_platform_file_stamp(COPY_ON_DISK, &stamp) ? stamp : 0;
}

// A new store the world is updated into, the game's way when `cooked`, with
// no failure and the `.glb` loaded at `stamp`.
static voe_3d_models *store_updated(const voe_ecs_world *world,
				    voe_render_device *device,
				    voe_base_arena *scratch, bool cooked,
				    uint64_t stamp)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures =
		cooked ? voe_game_models_update_cooked(world, models, device,
						       FOLDER, scratch, NULL) :
			 voe_game_models_update(world, models, device, FOLDER,
						scratch, NULL);
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GOOD);

	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->stamp == stamp);
	return models;
}

static void store_gone(voe_3d_models *models, voe_render_device *device)
{
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

static void update_writes_the_copy(const voe_ecs_world *world,
				   voe_render_device *device,
				   voe_base_arena *scratch)
{
	uint64_t stamp = glb_write(0);
	voe_3d_models *models;

	remove(COPY_ON_DISK);
	models = store_updated(world, device, scratch, false, stamp);
	VOE_TEST_CHECK(copy_key().glb == stamp);
	VOE_TEST_CHECK(copy_key().parts == 0);
	store_gone(models, device);
}

static void a_fresh_copy_is_read_not_written_again(const voe_ecs_world *world,
						   voe_render_device *device,
						   voe_base_arena *scratch)
{
	uint64_t stamp = 0;
	uint64_t before = copy_stamp();
	voe_3d_models *models;

	VOE_TEST_CHECK(before != 0);
	VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &stamp));
	models = store_updated(world, device, scratch, false, stamp);
	VOE_TEST_CHECK(copy_stamp() == before);
	store_gone(models, device);
}

static void a_changed_glb_writes_it_again(const voe_ecs_world *world,
					  voe_render_device *device,
					  voe_base_arena *scratch)
{
	uint64_t before = copy_key().glb;
	uint64_t stamp = glb_write(4);
	voe_3d_models *models;

	VOE_TEST_CHECK(stamp != before);
	models = store_updated(world, device, scratch, false, stamp);
	VOE_TEST_CHECK(copy_key().glb == stamp);
	store_gone(models, device);
}

static void watch_reloads_and_writes_no_copy(const voe_ecs_world *world,
					     voe_render_device *device,
					     voe_base_arena *scratch)
{
	uint64_t stamp = 0;
	voe_3d_models *models;
	voe_game_models_failures failures;
	const voe_3d_model_entry *entry;

	VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &stamp));
	models = store_updated(world, device, scratch, false, stamp);
	remove(COPY_ON_DISK);
	stamp = glb_write(8);
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	entry = voe_3d_models_find(models, GOOD);
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->stamp == stamp);
	VOE_TEST_CHECK(!voe_platform_file_exists(COPY_ON_DISK));
	store_gone(models, device);
}

static void
update_cooked_loads_a_copy_whose_glb_is_garbage(const voe_ecs_world *world,
						voe_render_device *device,
						voe_base_arena *scratch)
{
	static const uint8_t GARBAGE[] = "these bytes are no glb";
	uint64_t stamp = glb_write(0);
	uint64_t garbage = 0;
	voe_3d_models *models;

	// ---- a copy written by an update
	remove(COPY_ON_DISK);
	store_gone(store_updated(world, device, scratch, false, stamp), device);
	VOE_TEST_CHECK(copy_key().glb == stamp);

	// ---- the `.glb` garbage: the game's update loads the copy, kept
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, GARBAGE,
					       sizeof(GARBAGE) - 1, NULL));
	VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &garbage));
	models = store_updated(world, device, scratch, true, garbage);
	VOE_TEST_CHECK(copy_key().glb == stamp);
	store_gone(models, device);
}

static void prepare_writes_the_copy(voe_base_arena *scratch)
{
	uint64_t stamp = glb_write(0);
	voe_base_error error = VOE_BASE_OK;
	bool made = false;

	// ---- a first prepare leaves both files fresh
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, scratch, false,
					       &made, &error));
	VOE_TEST_CHECK(voe_platform_file_exists(LEVELS_ON_DISK));

	// ---- the copy gone: made again, keyed by the stamp
	remove(COPY_ON_DISK);
	made = false;
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, scratch, false,
					       &made, &error));
	VOE_TEST_CHECK(made);
	VOE_TEST_CHECK(copy_key().glb == stamp);

	// ---- both fresh: nothing made
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, scratch, false,
					       &made, &error));
	VOE_TEST_CHECK(!made);
}

// The 16×16 PNG written, a gradient, its stamp returned; scratch rewound.
static uint64_t png_write(voe_base_arena *scratch)
{
	static uint8_t pixels[PICTURE_SIDE * PICTURE_SIDE * 4];
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_assets_image image = { PICTURE_SIDE, PICTURE_SIDE, pixels, 0,
				   VOE_ASSETS_IMAGE_RGBA8 };
	voe_assets_bytes png = { 0 };
	uint64_t stamp = 0;

	for (uint32_t i = 0; i < sizeof(pixels); i++)
		pixels[i] = (uint8_t)(i % 4 == 3 ? 255 : i);
	VOE_TEST_CHECK(voe_assets_png_encode(scratch, image, &png, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(PICTURE_ON_DISK, png.bytes,
					       png.count, NULL));
	VOE_TEST_CHECK(voe_platform_file_stamp(PICTURE_ON_DISK, &stamp));
	voe_base_arena_rewind(scratch, mark);
	return stamp;
}

// The map's copy's key from its head; 0 when there is none or it will not
// read.
static uint64_t map_copy_key(void)
{
	uint8_t head[VOE_ASSETS_IMAGE_COOKED_HEAD];
	uint64_t key = 0;
	size_t count = 0;

	if (!voe_platform_file_read_head(MAP_COPY_ON_DISK, head, sizeof(head),
					 &count, NULL) ||
	    !voe_assets_image_cooked_key(head, count, &key))
		return 0;
	return key;
}

// A grey cube wearing MATERIAL.
static void shade(voe_ecs_world *world)
{
	voe_3d_shape shape = { .kind = VOE_3D_SHAPE_CUBE,
			       .colour = VOE_3D_SHAPE_GREY };

	snprintf(shape.material, sizeof shape.material, "%s", MATERIAL);
	VOE_TEST_CHECK(voe_3d_shape_add(world, place(world), shape));
}

// A new store the world's materials are loaded into, the game's way when
// `cooked`, with no failure and MATERIAL loaded.
static voe_3d_models *store_shaded(const voe_ecs_world *world,
				   voe_render_device *device,
				   voe_base_arena *scratch, bool cooked)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures =
		cooked ? voe_game_models_materials_cooked(world, models, device,
							  FOLDER, &MATERIALS,
							  &NO_PARTS, scratch) :
			 voe_game_models_materials(world, models, device, FOLDER,
						   &MATERIALS, &NO_PARTS,
						   scratch);
	const voe_3d_model_entry *entry = voe_3d_models_find(models, MATERIAL);

	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->material);
	return models;
}

// The map copy `file` reads back as a full chain of `kind` in `format`, keyed
// `stamp`. Scratch rewound.
static void map_copy_check(const char *file, voe_assets_image_kind kind,
			   voe_assets_image_format format, uint64_t stamp,
			   voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_assets_image image = { 0 };
	voe_assets_image_kind kept = VOE_ASSETS_IMAGE_DATA;
	const uint8_t *bytes;
	uint64_t key = 0;
	size_t size = 0;
	size_t used = 0;

	bytes = voe_platform_file_read(file, scratch, &size, NULL);
	VOE_TEST_CHECK(bytes != NULL);
	if (bytes != NULL)
		VOE_TEST_CHECK(voe_assets_image_cooked_read(
			bytes, size, &image, &kept, &key, &used));
	VOE_TEST_CHECK_INT(image.levels,
			   voe_assets_image_level_count(PICTURE_SIDE,
							PICTURE_SIDE));
	VOE_TEST_CHECK_INT(image.width, PICTURE_SIDE);
	VOE_TEST_CHECK_INT(image.format, format);
	VOE_TEST_CHECK(kept == kind);
	VOE_TEST_CHECK(key == stamp);
	voe_base_arena_rewind(scratch, mark);
}

// The three map copies read back BC7, the normal's BC5, keyed `stamp`.
static void map_copies_check(uint64_t stamp, voe_base_arena *scratch)
{
	map_copy_check(MAP_COPY_ON_DISK, VOE_ASSETS_IMAGE_COLOUR,
		       VOE_ASSETS_IMAGE_BC7, stamp, scratch);
	map_copy_check(NORMAL_COPY_ON_DISK, VOE_ASSETS_IMAGE_NORMAL,
		       VOE_ASSETS_IMAGE_BC5, stamp, scratch);
	map_copy_check(DATA_COPY_ON_DISK, VOE_ASSETS_IMAGE_DATA,
		       VOE_ASSETS_IMAGE_BC7, stamp, scratch);
}

static void materials_write_the_map_copy(const voe_ecs_world *world,
					 voe_render_device *device,
					 voe_base_arena *scratch)
{
	uint64_t stamp = png_write(scratch);

	remove(MAP_COPY_ON_DISK);
	remove(NORMAL_COPY_ON_DISK);
	remove(DATA_COPY_ON_DISK);
	store_gone(store_shaded(world, device, scratch, false), device);
	map_copies_check(stamp, scratch);
}

static void material_load_writes_no_copy(voe_render_device *device,
					 voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_base_error error = VOE_BASE_OK;

	remove(MAP_COPY_ON_DISK);
	VOE_TEST_CHECK(voe_game_models_material_load(models, device, FOLDER,
						     &MATERIALS.materials[0], 0,
						     scratch, &error));
	VOE_TEST_CHECK(!voe_platform_file_exists(MAP_COPY_ON_DISK));
	store_gone(models, device);
}

static void
materials_cooked_loads_a_copy_whose_png_is_garbage(const voe_ecs_world *world,
						   voe_render_device *device,
						   voe_base_arena *scratch)
{
	static const uint8_t GARBAGE[] = "these bytes are no png";
	uint64_t stamp = png_write(scratch);

	// ---- a copy written by the materials call
	remove(MAP_COPY_ON_DISK);
	store_gone(store_shaded(world, device, scratch, false), device);
	VOE_TEST_CHECK(map_copy_key() == stamp);

	// ---- the PNG garbage: the game's call loads the copy, kept
	VOE_TEST_CHECK(voe_platform_file_write(PICTURE_ON_DISK, GARBAGE,
					       sizeof(GARBAGE) - 1, NULL));
	store_gone(store_shaded(world, device, scratch, true), device);
	VOE_TEST_CHECK(map_copy_key() == stamp);
}

// The `.glb`s the editor's check is given.
static const char *const GLBS[] = { GOOD };

// How many copies the check finds stale.
static unsigned stale(voe_base_arena *scratch)
{
	return voe_game_models_stale(FOLDER, GLBS, 1, &MATERIALS, scratch);
}

static void stale_counts_missing_copies(voe_base_arena *scratch)
{
	(void)glb_write(0);
	(void)png_write(scratch);
	remove(COPY_ON_DISK);
	remove(MAP_COPY_ON_DISK);
	remove(NORMAL_COPY_ON_DISK);
	remove(DATA_COPY_ON_DISK);
	VOE_TEST_CHECK_INT(stale(scratch), 4);
}

static void cook_makes_them_and_stale_is_then_0(voe_base_arena *scratch)
{
	voe_game_progress progress = { 0 };
	uint64_t glb = 0;
	uint64_t png = 0;

	VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &glb));
	VOE_TEST_CHECK(voe_platform_file_stamp(PICTURE_ON_DISK, &png));
	VOE_TEST_CHECK(voe_game_models_cook(FOLDER, GLBS, 1, &MATERIALS,
					    scratch, &progress));
	VOE_TEST_CHECK(copy_key().glb == glb);
	map_copies_check(png, scratch);
	VOE_TEST_CHECK_INT(atomic_load(&progress.done), 4);
	VOE_TEST_CHECK_INT(atomic_load(&progress.total), 4);
	VOE_TEST_CHECK_INT(stale(scratch), 0);
}

// The colour copy rewritten with the version before 0477's, 1, at its head's
// fifth byte: stale until a cook makes it again. Scratch rewound.
static void an_old_map_copy_counts_stale(voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_game_progress progress = { 0 };
	const uint32_t old = 1;
	const uint8_t *bytes;
	uint8_t *copy;
	size_t size = 0;

	bytes = voe_platform_file_read(MAP_COPY_ON_DISK, scratch, &size, NULL);
	VOE_TEST_CHECK(bytes != NULL && size >= VOE_ASSETS_IMAGE_COOKED_HEAD);
	if (bytes != NULL && size >= VOE_ASSETS_IMAGE_COOKED_HEAD) {
		copy = voe_base_arena_push(scratch, size);
		memcpy(copy, bytes, size);
		memcpy(copy + 4, &old, sizeof(old));
		VOE_TEST_CHECK(voe_platform_file_write(MAP_COPY_ON_DISK, copy,
						       size, NULL));
	}
	voe_base_arena_rewind(scratch, mark);
	VOE_TEST_CHECK_INT(stale(scratch), 1);
	VOE_TEST_CHECK(voe_game_models_cook(FOLDER, GLBS, 1, &MATERIALS,
					    scratch, &progress));
	VOE_TEST_CHECK_INT(stale(scratch), 0);
}

static void stale_counts_a_changed_glb(voe_base_arena *scratch)
{
	(void)glb_write(4);
	VOE_TEST_CHECK_INT(stale(scratch), 1);
}

static void cook_stops_when_asked(voe_base_arena *scratch)
{
	voe_game_progress progress = { 0 };
	uint64_t before = copy_key().glb;

	atomic_store(&progress.stop, true);
	VOE_TEST_CHECK(!voe_game_models_cook(FOLDER, GLBS, 1, &MATERIALS,
					     scratch, &progress));
	VOE_TEST_CHECK(copy_key().glb == before);
	VOE_TEST_CHECK_INT(stale(scratch), 1);
}

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(GOOD_ON_DISK);
	remove(PICTURE_ON_DISK);
	remove(COPY_ON_DISK);
	remove(MAP_COPY_ON_DISK);
	remove(NORMAL_COPY_ON_DISK);
	remove(DATA_COPY_ON_DISK);
	remove(PICTURES_FOLDER_ON_DISK);
	remove(MODELS_FOLDER_ON_DISK);
	remove(LEVELS_ON_DISK);
	remove(LEVELS_FOLDER_ON_DISK);
	remove(CACHE_ON_DISK);
	remove(FOLDER);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_base_arena *scratch = voe_base_arena_new(1 << 16);
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	voe_ecs_world *world;

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		goto released;
	}
	// A folder left by a run that crashed goes first.
	remove_files();
	VOE_TEST_CHECK(voe_platform_folder_create(FOLDER, NULL));
	world = voe_game_world_new(arena);
	wear(world, GOOD);
	shade(world);
	update_writes_the_copy(world, device, scratch);
	a_fresh_copy_is_read_not_written_again(world, device, scratch);
	a_changed_glb_writes_it_again(world, device, scratch);
	watch_reloads_and_writes_no_copy(world, device, scratch);
	update_cooked_loads_a_copy_whose_glb_is_garbage(world, device, scratch);
	prepare_writes_the_copy(scratch);
	materials_write_the_map_copy(world, device, scratch);
	material_load_writes_no_copy(device, scratch);
	materials_cooked_loads_a_copy_whose_png_is_garbage(world, device,
							   scratch);
	stale_counts_missing_copies(scratch);
	cook_makes_them_and_stale_is_then_0(scratch);
	an_old_map_copy_counts_stale(scratch);
	stale_counts_a_changed_glb(scratch);
	cook_stops_when_asked(scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
