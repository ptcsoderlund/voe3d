// The model loader against real files: a one-triangle `.glb` built here, JSON
// and BIN chunks by hand, written into a scratch folder in the working
// directory (the build tree), and a game world whose model rows name it.
//
// THE CASES, in order on one store: a row naming the file updates with no
// failure and a loaded entry; a second row naming a missing file gives one
// failure, named, while the first is left alone; the file rewritten as text
// makes a watch report one failure and keeps the entry loaded; rewritten
// valid, a watch reports none and the entry is loaded at the new stamp.
//
// The files and the folder are removed at the end, pass or fail. It skips
// when there is no graphics card, because a load uploads.
#include <game/models.h>
#include <game/world.h>

#include <3d/model_component.h>
#include <3d/models.h>

#include <base/arena.h>
#include <base/error.h>

#include <math/quat.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FOLDER "game_models_test"
#define GOOD "good.glb"
#define MISSING "missing.glb"
#define GOOD_ON_DISK FOLDER "/" GOOD

static const voe_render_capacities CAPACITIES = {
	.vertices = 8, .indices = 8, .geometries = 4,
	.objects = 1,  .shadings = 4, .passes = 1,
};

static const char JSON[] =
	"{\"asset\":{\"version\":\"2.0\"},\"nodes\":[{\"mesh\":0}],"
	"\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0}}]}],"
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

// The `.glb` into `out`, its length returned: a 12-byte header, the JSON
// chunk padded with spaces to four bytes, then the BIN chunk.
static uint32_t glb_build(uint8_t *out, size_t room)
{
	uint32_t json = (uint32_t)(sizeof(JSON) - 1 + 3) & ~3u;
	uint32_t total = 12 + 8 + json + 8 + (uint32_t)sizeof(TRIANGLE);

	VOE_TEST_CHECK(total <= room);
	put_u32(out, 0x46546C67u);
	put_u32(out + 4, 2);
	put_u32(out + 8, total);
	put_u32(out + 12, json);
	put_u32(out + 16, 0x4E4F534Au);
	memset(out + 20, ' ', json);
	memcpy(out + 20, JSON, sizeof(JSON) - 1);
	put_u32(out + 20 + json, (uint32_t)sizeof(TRIANGLE));
	put_u32(out + 24 + json, 0x004E4942u);
	memcpy(out + 28 + json, TRIANGLE, sizeof(TRIANGLE));
	return total;
}

// A thing at the origin wearing `path`.
static void wear(voe_ecs_world *world, const char *path)
{
	voe_scene_transform pose = {
		.rotation = voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, 0.0f),
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_3d_model model = { 0 };
	voe_ecs_entity thing;

	strcpy(model.path, path);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing, pose));
	VOE_TEST_CHECK(voe_3d_model_add(world, thing, model));
}

static bool loaded_at(const voe_3d_models *models, uint64_t *stamp)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GOOD);

	if (entry == NULL)
		return false;
	*stamp = entry->stamp;
	return entry->loaded;
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
	failures = voe_game_models_update(world, models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(failures.first == NULL);
	VOE_TEST_CHECK(loaded_at(models, &first));

	// ---- a row naming a missing file: one failure, named
	wear(world, MISSING);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch);
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

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_base_arena *scratch = voe_base_arena_new(1 << 16);
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
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
	remove(GOOD_ON_DISK);
	remove(FOLDER);
	VOE_TEST_CHECK(voe_platform_folder_create(FOLDER, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, glb, length, NULL));
	check_files(voe_game_world_new(arena), device, scratch, glb, length);
	remove(GOOD_ON_DISK);
	remove(FOLDER);
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
