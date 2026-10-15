// The model loader against real files: a one-triangle `.glb` built here, JSON
// and BIN chunks by hand, written into a scratch folder in the working
// directory (the build tree), and a game world whose model rows name it.
//
// THE CASES, in order on one store: a row naming the file updates with no
// failure and a loaded entry; a second row naming a missing file gives one
// failure, named, while the first is left alone; the file rewritten as text
// makes a watch report one failure and keeps the entry loaded; rewritten
// valid, a watch reports none and the entry is loaded at the new stamp. That
// world has no emitter, so no dot is loaded.
//
// THE PICTURES, on a second world and store: an emitter naming a 1x1 PNG,
// inline bytes, loads a picture entry at
// its path and the dot at ""; a second emitter naming a missing file is one
// failure, named.
//
// THE WATER, on a third world and store: with no water an update loads no
// water record; with one, it loads it.
//
// THE PROGRESS, on a fourth world and store: two rows naming the `.glb` update
// with a progress to total 1 and done 1; with stop already asked an update
// reads nothing, and a second one without a progress loads the file.
//
// THE LANDSCAPES, on a fifth store: a 4-cell table loads at stamp 0 with its
// millimetres as metres, one part; with a text file at its path a watch reports nothing
// and leaves it loaded at 0.
//
// THE MATERIALS, on a sixth world and store: a shape naming a table material
// whose colour map is the PNG loads it, `material`; a shape naming a path not
// in the table is one failure, named, and a second call counts none; with the
// material on disk as text a watch reports nothing and leaves it loaded.
//
// THE LAYERS, on a seventh store and an empty world: a landscape whose two
// layers name table materials loads both, holds both paths and its painted
// point's bytes as cooked; a layer naming no table material is one failure.
//
// THE PARTS, on an eighth world and store: a parts table naming a table
// material for the `.glb`'s material 0 makes its part wear it and loads it; a
// table without the `.glb` leaves `wears` empty; a material with an emission
// map loads it into the emissive slot.
//
// THE LEVELS, on a ninth world and store: an update leaves the `.glb`'s levels
// file in the folder's Cache, fitting the `.glb`; a prepare then makes none;
// the file made stale, a prepare makes it again, fitting; a remake on the
// fresh file makes it again, fitting; a missing `.glb` will not prepare. Then
// a flat 8 × 8 grid, `grid.glb`, prepared with remake: with no `grid.parts`
// three levels or more; with one saying `count=2`, two; with `count=9`, which
// will not parse, three or more again. On a tenth world and store, the grid
// updated holds three levels or more; a prepare to `count=2` leaves the store
// alone; a reload holds two; a reload of a path the store lacks adds nothing.
//
// The files and the folder are removed at the end, pass or fail. It skips
// when there is no graphics card, because a load uploads.
#include <game/models.h>
#include <game/progress.h>
#include <game/world.h>

#include <3d/emitter_component.h>
#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <3d/models.h>
#include <3d/shape_component.h>
#include <3d/water_component.h>

#include <base/arena.h>
#include <base/error.h>

#include <math/quat.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FOLDER "game_models_test"
#define GOOD "good.glb"
#define MISSING "missing.glb"
#define GOOD_ON_DISK FOLDER "/" GOOD
#define PICTURE "puff.png"
#define NO_PICTURE "missing.png"
#define PICTURE_ON_DISK FOLDER "/" PICTURE
#define LANDSCAPE "hill.landscape"
#define LANDSCAPE_ON_DISK FOLDER "/" LANDSCAPE
#define MATERIAL "Assets/m.material"
#define NO_MATERIAL "Assets/none.material"
#define GREY_MATERIAL "Assets/grey.material"
#define GLOW_MATERIAL "Assets/glow.material"
#define LAYERED_LANDSCAPE "layered.landscape"
#define ASSETS_ON_DISK FOLDER "/Assets"
#define MATERIAL_ON_DISK FOLDER "/" MATERIAL
#define CACHE_ON_DISK FOLDER "/Cache"
#define LEVELS_FOLDER_ON_DISK CACHE_ON_DISK "/levels"
#define LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" GOOD ".levels"
#define GRID "grid.glb"
#define GRID_ON_DISK FOLDER "/" GRID
#define GRID_PARTS_ON_DISK FOLDER "/grid.parts"
#define GRID_LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" GRID ".levels"

// Room for the triangle twice over a reload, the pictures' quad, and the
// shadings of the triangle, the picture and the dot, plus two blended twins:
// one for each triangle part held at once over the reload (ADR-0336 point 2).
// Then a landscape as the store makes it (0396 point 3): the shared grid of
// VOE_3D_LANDSCAPE_NODE_QUADS² quads, its ground and its twin, and the
// heights texture, which takes a texture slot and no capacity. Then a
// material's record and twin, with no geometry, and a second one's. The
// reload case's store alone holds the most geometries: the grid's four levels
// and the two it reloads to, at once.
#define GRID_SIDE (VOE_3D_LANDSCAPE_NODE_QUADS + 1)
static const voe_render_capacities CAPACITIES = {
	.vertices = 16 + GRID_SIDE * GRID_SIDE,
	.indices = 16 + VOE_3D_LANDSCAPE_NODE_QUADS *
				VOE_3D_LANDSCAPE_NODE_QUADS * 6,
	.geometries = 4 + 2,
	.objects = 1,
	.shadings = 10 + 2 + 2 + 2,
	.passes = 1,
};

// A 4-cell, 64 m landscape: a 1.5 m bump in the middle, -0.25 m at a corner.
static const int32_t HILL[25] = {
	-250, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1500,
	0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const voe_game_landscapes HILLS = {
	(const voe_game_landscape[]){ { LANDSCAPE, 64.0f, 4, HILL, NULL, 0, NULL } },
	1
};

// A lit, white material with the PNG as its colour map, a grey one with no
// maps, and a glowing one with the PNG as its emission map.
static const voe_game_materials MATERIALS = {
	(const voe_game_material[]){ { MATERIAL,
				       { .colour = { 1.0f, 1.0f, 1.0f },
					 .roughness = 0.5f,
					 .repeat = 1.0f,
					 .colormap = PICTURE } },
				     { GREY_MATERIAL,
				       { .colour = { 0.5f, 0.5f, 0.5f },
					 .roughness = 0.5f,
					 .repeat = 1.0f } },
				     { GLOW_MATERIAL,
				       { .colour = { 1.0f, 1.0f, 1.0f },
					 .roughness = 0.5f,
					 .repeat = 1.0f,
					 .emission = { 1.0f, 1.0f, 1.0f },
					 .emission_strength = 1.0f,
					 .emissionmap = PICTURE } } },
	3
};

// No model's parts; the `.glb`'s material 0 wearing MATERIAL.
static const voe_game_parts NO_PARTS = { NULL, 0 };
static const voe_game_parts PARTS = {
	(const voe_game_model_parts[]){
		{ GOOD, { .count = 1, .materials = { MATERIAL } } } },
	1
};

// One plane of a 4-cell landscape's paint: all layer 0 but the middle point,
// 200 of layer 1. Filled by main, since a literal would be 100 bytes.
#define PAINTED 12
static uint8_t paint[25 * 4];

static const voe_game_landscapes LAYERED = {
	(const voe_game_landscape[]){
		{ LAYERED_LANDSCAPE, 64.0f, 4, HILL,
		  (const char *const[]){ MATERIAL, GREY_MATERIAL }, 2, paint } },
	1
};

static const voe_game_landscapes UNKNOWN_LAYER = {
	(const voe_game_landscape[]){
		{ LAYERED_LANDSCAPE, 64.0f, 4, HILL,
		  (const char *const[]){ NO_MATERIAL }, 1, NULL } },
	1
};

// A 1x1 white RGBA PNG: signature, IHDR, one zlib IDAT row, IEND.
static const uint8_t PNG[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00,
	0x0b, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8, 0x0f, 0x04, 0x00,
	0x09, 0xfb, 0x03, 0xfd, 0xfb, 0x5e, 0x6b, 0x2b, 0x00, 0x00, 0x00, 0x00,
	0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

static const char JSON[] =
	"{\"asset\":{\"version\":\"2.0\"},\"nodes\":[{\"mesh\":0}],"
	"\"materials\":[{}],"
	"\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},"
	"\"material\":0}]}],"
	"\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,"
	"\"type\":\"VEC3\",\"min\":[0,0,0],\"max\":[1,1,0]}],"
	"\"bufferViews\":[{\"buffer\":0,\"byteLength\":36}],"
	"\"buffers\":[{\"byteLength\":36}]}";

static const float TRIANGLE[9] = { 0, 0, 0, 1, 0, 0, 0, 1, 0 };

static const uint8_t NOT_A_GLB[] = "this is not a model";

static void put_u32(uint8_t *at, uint32_t value)
{
	for (uint32_t i = 0; i < 4; i++)
		at[i] = (uint8_t)(value >> (8 * i));
}

// A `.glb` of `text` and `bin` into `out`, its length returned: a 12-byte
// header, the JSON chunk padded with spaces to four bytes, then the BIN chunk,
// whose size is a multiple of four.
static uint32_t glb_pack(uint8_t *out, size_t room, const char *text,
			 const void *bin, uint32_t bin_size)
{
	uint32_t length = (uint32_t)strlen(text);
	uint32_t json = (length + 3) & ~3u;
	uint32_t total = 12 + 8 + json + 8 + bin_size;

	VOE_TEST_CHECK(total <= room);
	if (total > room)
		return 0;
	put_u32(out, 0x46546C67u);
	put_u32(out + 4, 2);
	put_u32(out + 8, total);
	put_u32(out + 12, json);
	put_u32(out + 16, 0x4E4F534Au);
	memset(out + 20, ' ', json);
	memcpy(out + 20, text, length);
	put_u32(out + 20 + json, bin_size);
	put_u32(out + 24 + json, 0x004E4942u);
	memcpy(out + 28 + json, bin, bin_size);
	return total;
}

// The one-triangle `.glb` into `out`, its length returned.
static uint32_t glb_build(uint8_t *out, size_t room)
{
	return glb_pack(out, room, JSON, TRIANGLE, (uint32_t)sizeof(TRIANGLE));
}

// A flat grid of GRID_QUADS × GRID_QUADS unit quads in the plane z = 0: 81
// positions, then 384 32-bit indices in their own buffer view.
#define GRID_QUADS 8
#define GRID_POINTS ((GRID_QUADS + 1) * (GRID_QUADS + 1))
#define GRID_INDICES (GRID_QUADS * GRID_QUADS * 6)
static const char GRID_JSON[] =
	"{\"asset\":{\"version\":\"2.0\"},\"nodes\":[{\"mesh\":0}],"
	"\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},"
	"\"indices\":1}]}],"
	"\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":81,"
	"\"type\":\"VEC3\",\"min\":[0,0,0],\"max\":[8,8,0]},"
	"{\"bufferView\":1,\"componentType\":5125,\"count\":384,"
	"\"type\":\"SCALAR\"}],"
	"\"bufferViews\":[{\"buffer\":0,\"byteLength\":972},"
	"{\"buffer\":0,\"byteOffset\":972,\"byteLength\":1536}],"
	"\"buffers\":[{\"byteLength\":2508}]}";

// The grid's `.glb` into `out`, its length returned.
static uint32_t grid_build(uint8_t *out, size_t room)
{
	struct {
		float positions[GRID_POINTS * 3];
		uint32_t indices[GRID_INDICES];
	} bin;
	uint32_t at = 0;

	static_assert(sizeof(bin) == 2508, "the grid's BIN as its JSON says");
	for (uint32_t i = 0; i < GRID_POINTS; i++) {
		bin.positions[i * 3] = (float)(i % (GRID_QUADS + 1));
		bin.positions[i * 3 + 1] = (float)(i / (GRID_QUADS + 1));
		bin.positions[i * 3 + 2] = 0.0f;
	}
	for (uint32_t y = 0; y < GRID_QUADS; y++) {
		for (uint32_t x = 0; x < GRID_QUADS; x++) {
			uint32_t corner = y * (GRID_QUADS + 1) + x;
			uint32_t quad[6] = { corner, corner + 1,
					     corner + GRID_QUADS + 2, corner,
					     corner + GRID_QUADS + 2,
					     corner + GRID_QUADS + 1 };

			memcpy(bin.indices + at, quad, sizeof(quad));
			at += 6;
		}
	}
	return glb_pack(out, room, GRID_JSON, &bin, (uint32_t)sizeof(bin));
}

// A new thing at the origin.
static voe_ecs_entity place(voe_ecs_world *world)
{
	voe_scene_transform pose = {
		.rotation = voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, 0.0f),
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_ecs_entity thing;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing, pose));
	return thing;
}

// A thing at the origin wearing `path`.
static void wear(voe_ecs_world *world, const char *path)
{
	voe_3d_model model = { 0 };

	snprintf(model.path, sizeof model.path, "%s", path);
	VOE_TEST_CHECK(voe_3d_model_add(world, place(world), model));
}

// A thing at the origin emitting with the texture `path`.
static void emit(voe_ecs_world *world, const char *path)
{
	voe_3d_emitter emitter = { .playing = true };

	snprintf(emitter.texture, sizeof emitter.texture, "%s", path);
	VOE_TEST_CHECK(voe_3d_emitter_add(world, place(world), emitter));
}

static void check_pictures(voe_ecs_world *world, voe_render_device *device,
			   voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	const voe_3d_model_entry *entry;
	voe_game_models_failures failures;

	// ---- an emitter naming a PNG: a picture entry and the dot
	emit(world, PICTURE);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	entry = voe_3d_models_find(models, PICTURE);
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->picture);
	entry = voe_3d_models_find(models, "");
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->picture);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 1);

	// ---- an emitter naming a missing file: one failure, named
	emit(world, NO_PICTURE);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, NO_PICTURE) == 0);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

static void check_water(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;

	// ---- no water: no record
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(voe_3d_models_water(models) == NULL);

	// ---- one water: the record
	VOE_TEST_CHECK(voe_3d_water_add(world, place(world),
					(voe_3d_water){ .width = 20.0f,
							.length = 20.0f }));
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(voe_3d_models_water(models) != NULL);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

static bool loaded_at(const voe_3d_models *models, uint64_t *stamp)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GOOD);

	if (entry == NULL)
		return false;
	*stamp = entry->stamp;
	return entry->loaded;
}

static void check_progress(voe_ecs_world *world, voe_render_device *device,
			   voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_progress progress = { 0 };
	voe_game_models_failures failures;
	uint64_t stamp = 0;

	// ---- with a progress: one file counted, and done at the end
	wear(world, GOOD);
	wear(world, GOOD);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  &progress);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(loaded_at(models, &stamp));
	VOE_TEST_CHECK_INT(atomic_load(&progress.total), 1);
	VOE_TEST_CHECK_INT(atomic_load(&progress.done), 1);
	voe_3d_models_clear(models, device);

	// ---- stop already asked: nothing read; a later call reads it
	atomic_store(&progress.stop, true);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  &progress);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 0);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(loaded_at(models, &stamp));

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// The table's landscape held, loaded at stamp 0, heights in metres.
static void check_hill(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, LANDSCAPE);

	VOE_TEST_CHECK(entry != NULL && entry->loaded &&
		       entry->landscape != NULL);
	if (entry == NULL || entry->landscape == NULL)
		return;
	VOE_TEST_CHECK_INT(entry->stamp, 0);
	VOE_TEST_CHECK_INT(entry->part_count, 1);
	VOE_TEST_CHECK_INT(entry->landscape->cells, 4);
	VOE_TEST_CHECK(entry->landscape->size == 64.0f);
	VOE_TEST_CHECK(entry->landscape->heights[0] == -0.25f);
	VOE_TEST_CHECK(entry->landscape->heights[12] == 1.5f);
	VOE_TEST_CHECK(entry->landscape->heights[24] == 0.0f);
}

static void landscapes_load_from_a_table(voe_3d_models *models,
					 voe_render_device *device,
					 voe_base_arena *scratch)
{
	voe_game_models_failures failures =
		voe_game_models_landscapes(models, device, &HILLS, scratch);

	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(failures.first == NULL);
	check_hill(models);
}

// A text file at the landscape's path would fail a watch that read it.
static void watch_leaves_a_landscape_alone(voe_3d_models *models,
					   voe_render_device *device,
					   voe_base_arena *scratch)
{
	voe_game_models_failures failures;

	VOE_TEST_CHECK(voe_platform_file_write(LANDSCAPE_ON_DISK, NOT_A_GLB,
					       sizeof(NOT_A_GLB), NULL));
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	check_hill(models);
}

// A grey cube at the origin wearing the material `path`.
static void shade(voe_ecs_world *world, const char *path)
{
	voe_3d_shape shape = { .kind = VOE_3D_SHAPE_CUBE,
			       .colour = VOE_3D_SHAPE_GREY,
			       .cast_shadows = true };

	snprintf(shape.material, sizeof shape.material, "%s", path);
	VOE_TEST_CHECK(voe_3d_shape_add(world, place(world), shape));
}

static bool material_loaded(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, MATERIAL);

	return entry != NULL && entry->loaded && entry->material;
}

static void check_materials(voe_ecs_world *world, voe_render_device *device,
			    voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;

	// ---- a shape naming a table material: loaded, a material
	shade(world, MATERIAL);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(material_loaded(models));

	// ---- a path not in the table: one failure, named, then none
	shade(world, NO_MATERIAL);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, NO_MATERIAL) == 0);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);

	// ---- the material on disk as text: a watch leaves it alone
	VOE_TEST_CHECK(voe_platform_folder_create(ASSETS_ON_DISK, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(MATERIAL_ON_DISK, NOT_A_GLB,
					       sizeof(NOT_A_GLB), NULL));
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(material_loaded(models));

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// `path` held in `models` as a loaded material.
static bool holds_material(const voe_3d_models *models, const char *path)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, path);

	return entry != NULL && entry->loaded && entry->material;
}

// The layered landscape's grid: both layer paths and the painted point's
// bytes as cooked, the second plane nought.
static void check_layered(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry =
		voe_3d_models_find(models, LAYERED_LANDSCAPE);
	const uint8_t *point;

	VOE_TEST_CHECK(entry != NULL && entry->landscape != NULL);
	if (entry == NULL || entry->landscape == NULL)
		return;
	VOE_TEST_CHECK_INT(entry->landscape->layer_count, 2);
	VOE_TEST_CHECK(strcmp(entry->landscape->layers[0], MATERIAL) == 0);
	VOE_TEST_CHECK(strcmp(entry->landscape->layers[1], GREY_MATERIAL) == 0);
	VOE_TEST_CHECK(entry->landscape->paint != NULL);
	if (entry->landscape->paint == NULL)
		return;
	point = entry->landscape->paint + PAINTED * 4;
	VOE_TEST_CHECK(memcmp(point, paint + PAINTED * 4, 4) == 0);
	VOE_TEST_CHECK(memcmp(point + sizeof(paint), (uint8_t[4]){ 0 }, 4) ==
		       0);
}

static void check_layers(voe_ecs_world *world, voe_render_device *device,
			 voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;

	for (uint32_t i = 0; i < 25; i++)
		paint[i * 4] = 255;
	paint[PAINTED * 4] = 55;
	paint[PAINTED * 4 + 1] = 200;

	// ---- two layers naming table materials: both loaded, the paint kept
	failures = voe_game_models_landscapes(models, device, &LAYERED, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(holds_material(models, MATERIAL));
	VOE_TEST_CHECK(holds_material(models, GREY_MATERIAL));
	check_layered(models);
	voe_3d_models_clear(models, device);

	// ---- a layer naming no table material: one failure, named
	failures = voe_game_models_landscapes(models, device, &UNKNOWN_LAYER,
					      scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, NO_MATERIAL) == 0);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// The `.glb`'s part 0 wearing `path`.
static bool wears(const voe_3d_models *models, const char *path)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GOOD);

	return entry != NULL && entry->loaded && entry->part_count == 1 &&
	       strcmp(entry->parts[0].wears, path) == 0;
}

static void check_parts(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	const voe_3d_model_entry *glow;
	voe_game_models_failures failures;

	// ---- a parts table naming a material for material 0: worn, loaded
	wear(world, GOOD);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &PARTS, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(wears(models, MATERIAL));
	VOE_TEST_CHECK(material_loaded(models));

	// ---- a table without the `.glb`: nothing worn
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(wears(models, ""));

	// ---- a material with an emission map: in the emissive slot
	shade(world, GLOW_MATERIAL);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	glow = voe_3d_models_find(models, GLOW_MATERIAL);
	VOE_TEST_CHECK(glow != NULL && glow->loaded && glow->material);
	if (glow != NULL && glow->loaded)
		VOE_TEST_CHECK(glow->parts[0].material.emissive_texture.index !=
			       VOE_RENDER_NO_TEXTURE);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

static void check_files(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *scratch, const uint8_t *glb,
			uint32_t size)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;
	uint64_t first = 0, stamp = 0;

	// ---- a row naming the file: no failure, a loaded entry
	wear(world, GOOD);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(failures.first == NULL);
	VOE_TEST_CHECK(loaded_at(models, &first));
	VOE_TEST_CHECK(voe_3d_models_find(models, "") == NULL);

	// ---- a row naming a missing file: one failure, named
	wear(world, MISSING);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, MISSING) == 0);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 2);

	// ---- the file rewritten as text: the watch fails it, still loaded
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, NOT_A_GLB,
					       sizeof(NOT_A_GLB), NULL));
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, GOOD) == 0);
	VOE_TEST_CHECK(loaded_at(models, &stamp));
	VOE_TEST_CHECK(stamp != first);

	// ---- rewritten valid: the watch loads it again
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, glb, size, NULL));
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(loaded_at(models, &first));
	VOE_TEST_CHECK(first != stamp);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// The levels file on disk read against the `.glb`'s bytes.
static bool levels_fit(voe_base_arena *scratch, const uint8_t *glb,
		       uint32_t size)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_3d_model_levels levels;
	size_t count = 0;
	const uint8_t *bytes =
		voe_platform_file_read(LEVELS_ON_DISK, scratch, &count, NULL);
	bool fits = bytes != NULL &&
		    voe_3d_model_levels_read(scratch, bytes, count, glb, size,
					     &levels);

	voe_base_arena_rewind(scratch, mark);
	return fits;
}

// The grid's levels file on disk read against `grid`: its level count, 0
// when it will not read.
static uint32_t grid_levels(voe_base_arena *scratch, const uint8_t *grid,
			    uint32_t size)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_3d_model_levels levels = { 0 };
	size_t count = 0;
	const uint8_t *bytes = voe_platform_file_read(GRID_LEVELS_ON_DISK,
						      scratch, &count, NULL);
	bool fits = bytes != NULL &&
		    voe_3d_model_levels_read(scratch, bytes, count, grid, size,
					     &levels);

	voe_base_arena_rewind(scratch, mark);
	return fits ? levels.level_count : 0;
}

// The grid prepared, remade, with `parts` as its parts file (NULL for none):
// true and made, its level count returned; 0 when it would not prepare.
static uint32_t grid_prepared(voe_base_arena *scratch, const uint8_t *grid,
			      uint32_t size, const char *parts)
{
	voe_base_error error = VOE_BASE_OK;
	bool made = false;
	bool ready;

	if (parts != NULL)
		VOE_TEST_CHECK(voe_platform_file_write(
			GRID_PARTS_ON_DISK, (const uint8_t *)parts,
			strlen(parts), NULL));
	ready = voe_game_models_prepare(FOLDER, GRID, scratch, true, &made,
					&error);
	VOE_TEST_CHECK(ready);
	VOE_TEST_CHECK(made);
	return ready ? grid_levels(scratch, grid, size) : 0;
}

// The grid's levels as its parts file says: the defaults with none, its
// count, and the defaults again for a count past the limit.
static void check_levels_asked(voe_base_arena *scratch)
{
	static uint8_t grid[4096];
	uint32_t size = grid_build(grid, sizeof(grid));

	VOE_TEST_CHECK(voe_platform_file_write(GRID_ON_DISK, grid, size, NULL));

	// ---- no parts file: the default count, 3 or more
	remove(GRID_PARTS_ON_DISK);
	VOE_TEST_CHECK(grid_prepared(scratch, grid, size, NULL) >= 3);

	// ---- `count=2`: two levels
	VOE_TEST_CHECK_INT(grid_prepared(scratch, grid, size,
					 "[Parts]\n[Levels]\ncount=2\n"),
			   2);

	// ---- `count=9` will not parse: the default again
	VOE_TEST_CHECK(grid_prepared(scratch, grid, size,
				     "[Parts]\n[Levels]\ncount=9\n") >= 3);
}

// The grid's entry's level count, 0 when the store lacks it.
static uint32_t grid_held(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GRID);

	return entry != NULL ? entry->level_count : 0;
}

// The store keeps the levels it loaded over a prepare; a reload takes the
// new ones.
static void check_reload(voe_ecs_world *world, voe_render_device *device,
			 voe_base_arena *scratch)
{
	static uint8_t grid[4096];
	uint32_t size = grid_build(grid, sizeof(grid));
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;
	voe_base_error error = VOE_BASE_OK;
	uint32_t held;
	uint32_t count;

	VOE_TEST_CHECK(voe_platform_file_write(GRID_ON_DISK, grid, size, NULL));

	// ---- no parts file, an update: 3 levels or more
	remove(GRID_PARTS_ON_DISK);
	wear(world, GRID);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	held = grid_held(models);
	VOE_TEST_CHECK(held >= 3);

	// ---- `count=2` prepared: the store's levels unchanged
	VOE_TEST_CHECK_INT(grid_prepared(scratch, grid, size,
					 "[Parts]\n[Levels]\ncount=2\n"),
			   2);
	VOE_TEST_CHECK_INT(grid_held(models), held);

	// ---- reloaded: two levels
	VOE_TEST_CHECK(voe_game_models_reload(models, device, FOLDER, GRID,
					      scratch, &error));
	VOE_TEST_CHECK_INT(grid_held(models), 2);

	// ---- a path the store lacks: true, nothing added
	count = voe_3d_models_count(models);
	VOE_TEST_CHECK(voe_game_models_reload(models, device, FOLDER, MISSING,
					      scratch, &error));
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), count);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

static void check_levels(voe_ecs_world *world, voe_render_device *device,
			 voe_base_arena *scratch, const uint8_t *glb,
			 uint32_t size)
{
	static const uint8_t STALE[] = "stale";
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;
	voe_base_error error = VOE_BASE_OK;
	bool made = true;

	// ---- an update leaves the levels file, fitting the `.glb`
	remove(LEVELS_ON_DISK);
	wear(world, GOOD);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(levels_fit(scratch, glb, size));

	// ---- prepare with the file there: none made
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, scratch, false,
					       &made, &error));
	VOE_TEST_CHECK(!made);

	// ---- the file stale: made again, fitting
	VOE_TEST_CHECK(voe_platform_file_write(LEVELS_ON_DISK, STALE,
					       sizeof(STALE) - 1, NULL));
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, scratch, false,
					       &made, &error));
	VOE_TEST_CHECK(made);
	VOE_TEST_CHECK(levels_fit(scratch, glb, size));

	// ---- a remake on the fresh file: made again, fitting
	made = false;
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, scratch, true,
					       &made, &error));
	VOE_TEST_CHECK(made);
	VOE_TEST_CHECK(levels_fit(scratch, glb, size));

	// ---- a missing `.glb`: false
	VOE_TEST_CHECK(!voe_game_models_prepare(FOLDER, MISSING, scratch, false,
						&made, &error));

	check_levels_asked(scratch);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(GOOD_ON_DISK);
	remove(PICTURE_ON_DISK);
	remove(LANDSCAPE_ON_DISK);
	remove(MATERIAL_ON_DISK);
	remove(ASSETS_ON_DISK);
	remove(LEVELS_ON_DISK);
	remove(GRID_ON_DISK);
	remove(GRID_PARTS_ON_DISK);
	remove(GRID_LEVELS_ON_DISK);
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
	voe_3d_models *landscapes;
	uint8_t glb[512];
	uint32_t length = glb_build(glb, sizeof(glb));

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
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, glb, length, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(PICTURE_ON_DISK, PNG,
					       sizeof(PNG), NULL));
	check_files(voe_game_world_new(arena), device, scratch, glb, length);
	check_pictures(voe_game_world_new(arena), device, scratch);
	check_water(voe_game_world_new(arena), device, scratch);
	check_progress(voe_game_world_new(arena), device, scratch);
	landscapes = voe_3d_models_new();
	landscapes_load_from_a_table(landscapes, device, scratch);
	watch_leaves_a_landscape_alone(landscapes, device, scratch);
	voe_3d_models_clear(landscapes, device);
	voe_3d_models_destroy(landscapes);
	check_materials(voe_game_world_new(arena), device, scratch);
	check_layers(voe_game_world_new(arena), device, scratch);
	check_parts(voe_game_world_new(arena), device, scratch);
	check_levels(voe_game_world_new(arena), device, scratch, glb, length);
	check_reload(voe_game_world_new(arena), device, scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
