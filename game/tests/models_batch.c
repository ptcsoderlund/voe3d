// The batch's bursts against real files (0487): three `.glb`s of glb_build.h's
// packer, each a triangle wearing two 16×16 PNGs of its own as its colour and
// normal maps, written into a scratch folder in the working directory (the
// build tree).
//
// a_splash_cook_matches_one_by_one: the three cooked once by the editor's cook
// with a progress, so in one batch of bursts on the splash's workers, then,
// their copies and levels files gone, prepared one at a time with none: each
// `Cache/models/<path>.model` is equal byte for byte to the first.
//
// a_batchs_first_error_is_its_first_failing_file: two `.glb`s that will not
// parse, loaded in one batch on workers: the first error the calling thread
// reads names the first of them (0490).
//
// The files and the folders are removed at the end, pass or fail. It skips
// when there is no graphics card, because a copy is compressed on it.
#include "glb_build.h"

#include <game/models.h>
#include <game/world.h>

#include <3d/model_levels.h>
#include <3d/models.h>

#include <assets/image.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FOLDER "game_models_batch_test"
#define CACHE_ON_DISK FOLDER "/Cache"
#define MODELS_FOLDER_ON_DISK CACHE_ON_DISK "/models"
#define LEVELS_FOLDER_ON_DISK CACHE_ON_DISK "/levels"
#define FILES 3
#define PICTURE_SIDE 16

static const char *const PATHS[FILES] = { "a.glb", "b.glb", "c.glb" };

// The device a cook compresses on; nothing is uploaded.
static const voe_render_capacities CAPACITIES = {
	.vertices = 64,
	.indices = 64,
	.geometries = VOE_3D_MODEL_LEVELS * 2,
	.objects = 1,
	.shadings = 16,
	.passes = 1,
};

// The two `.glb`s whose JSON will not parse, worn in this order.
static const char *const BROKEN[2] = { "first_broken.glb",
				       "second_broken.glb" };

// `<FOLDER>/<under>/<path><ending>` into `out`.
static const char *on_disk(char *out, size_t room, const char *under,
			   const char *path, const char *ending)
{
	snprintf(out, room, "%s%s/%s%s", FOLDER, under, path, ending);
	return out;
}

// A 16×16 PNG whose colours `seed` picks, encoded onto `scratch`.
static voe_assets_bytes png_of(uint32_t seed, voe_base_arena *scratch)
{
	static uint8_t pixels[PICTURE_SIDE * PICTURE_SIDE * 4];
	voe_assets_image image = { PICTURE_SIDE, PICTURE_SIDE, pixels, 0,
				   VOE_ASSETS_IMAGE_RGBA8 };
	voe_assets_bytes png = { 0 };

	for (uint32_t i = 0; i < sizeof(pixels); i++)
		pixels[i] = (uint8_t)(i % 4 == 3 ? 255 : i * seed);
	VOE_TEST_CHECK(voe_assets_png_encode(scratch, image, &png, NULL));
	return png;
}

// File `n`: the triangle with UVs, its colour and normal maps two PNGs of its
// own packed after them in the BIN chunk. Scratch rewound.
static void pictured_write(uint32_t n, voe_base_arena *scratch)
{
	static const float UVS[6] = { 0, 0, 1, 0, 0, 1 };
	static char json[2048];
	static uint8_t bin[8192];
	static uint8_t glb[16384];
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_assets_bytes colour = png_of(2 * n + 1, scratch);
	voe_assets_bytes normal = png_of(2 * n + 2, scratch);
	size_t end = 60 + colour.count + normal.count;
	char path[256];
	uint32_t length;

	VOE_TEST_CHECK(end <= sizeof(bin));
	if (end > sizeof(bin))
		return;
	memset(bin, 0, sizeof(bin));
	memcpy(bin, TRIANGLE, 36);
	memcpy(bin + 36, UVS, 24);
	memcpy(bin + 60, colour.bytes, colour.count);
	memcpy(bin + 60 + colour.count, normal.bytes, normal.count);
	snprintf(json, sizeof(json),
		 "{\"asset\":{\"version\":\"2.0\"},\"nodes\":[{\"mesh\":0}],"
		 "\"materials\":[{\"pbrMetallicRoughness\":"
		 "{\"baseColorTexture\":{\"index\":0}},"
		 "\"normalTexture\":{\"index\":1}}],"
		 "\"meshes\":[{\"primitives\":[{\"attributes\":"
		 "{\"POSITION\":0,\"TEXCOORD_0\":1},\"material\":0}]}],"
		 "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,"
		 "\"count\":3,\"type\":\"VEC3\",\"min\":[0,0,0],"
		 "\"max\":[1,1,0]},{\"bufferView\":1,\"componentType\":5126,"
		 "\"count\":3,\"type\":\"VEC2\"}],"
		 "\"bufferViews\":[{\"buffer\":0,\"byteLength\":36},"
		 "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":24},"
		 "{\"buffer\":0,\"byteOffset\":60,\"byteLength\":%u},"
		 "{\"buffer\":0,\"byteOffset\":%u,\"byteLength\":%u}],"
		 "\"buffers\":[{\"byteLength\":%u}],"
		 "\"images\":[{\"bufferView\":2,\"mimeType\":\"image/png\"},"
		 "{\"bufferView\":3,\"mimeType\":\"image/png\"}],"
		 "\"textures\":[{\"source\":0},{\"source\":1}]}",
		 (unsigned)colour.count, (unsigned)(60 + colour.count),
		 (unsigned)normal.count, (unsigned)end);
	length = glb_pack(glb, sizeof(glb), json, bin,
			  (uint32_t)((end + 3) & ~(size_t)3));
	VOE_TEST_CHECK(voe_platform_file_write(on_disk(path, sizeof(path), "",
						       PATHS[n], ""),
					       glb, length, NULL));
	voe_base_arena_rewind(scratch, mark);
}

// Every file's copy and levels file removed.
static void copies_gone(void)
{
	char path[256];

	for (uint32_t n = 0; n < FILES; n++) {
		remove(on_disk(path, sizeof(path), "/Cache/models", PATHS[n],
			       ".model"));
		remove(on_disk(path, sizeof(path), "/Cache/levels", PATHS[n],
			       ".levels"));
	}
}

static void a_splash_cook_matches_one_by_one(voe_render_device *device,
					     voe_base_arena *arena,
					     voe_base_arena *scratch)
{
	static const voe_game_materials NO_MATERIALS = { NULL, 0 };
	voe_game_progress progress = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const uint8_t *cooked[FILES];
	size_t sizes[FILES] = { 0 };
	char path[256];

	for (uint32_t n = 0; n < FILES; n++)
		pictured_write(n, scratch);
	copies_gone();
	VOE_TEST_CHECK(voe_game_models_cook(FOLDER, PATHS, FILES, &NO_MATERIALS,
					    device, scratch, &progress));
	VOE_TEST_CHECK_INT(atomic_load(&progress.done), FILES);
	for (uint32_t n = 0; n < FILES; n++) {
		cooked[n] = voe_platform_file_read(
			on_disk(path, sizeof(path), "/Cache/models", PATHS[n],
				".model"),
			arena, &sizes[n], NULL);
		VOE_TEST_CHECK(cooked[n] != NULL);
	}
	copies_gone();
	for (uint32_t n = 0; n < FILES; n++) {
		const uint8_t *alone;
		size_t size = 0;
		bool made = false;

		VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, PATHS[n], device,
						       scratch, false, &made,
						       &error));
		VOE_TEST_CHECK(made);
		alone = voe_platform_file_read(on_disk(path, sizeof(path),
						       "/Cache/models",
						       PATHS[n], ".model"),
					       arena, &size, NULL);
		VOE_TEST_CHECK(alone != NULL && cooked[n] != NULL);
		VOE_TEST_CHECK(size == sizes[n]);
		if (alone != NULL && cooked[n] != NULL && size == sizes[n])
			VOE_TEST_CHECK(memcmp(alone, cooked[n], size) == 0);
	}
}

// Both broken `.glb`s worn and loaded by one update given a progress, so in
// one batch on the splash's workers: after a clear, the first error read once
// it has failed begins with the path the batch holds first.
static void a_batchs_first_error_is_its_first_failing_file(
	voe_render_device *device, voe_base_arena *arena,
	voe_base_arena *scratch)
{
	static uint8_t glb[512];
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_3d_models *models = voe_3d_models_new();
	voe_game_progress progress = { 0 };
	voe_game_models_failures failures;
	uint32_t length = glb_pack(glb, sizeof(glb), "{\"asset\":", TRIANGLE,
				   (uint32_t)sizeof(TRIANGLE));
	const char *kept;
	char path[256];

	for (uint32_t n = 0; n < 2; n++) {
		VOE_TEST_CHECK(voe_platform_file_write(
			on_disk(path, sizeof(path), "", BROKEN[n], ""), glb,
			length, NULL));
		wear(world, BROKEN[n]);
	}
	voe_base_report_error_clear();
	failures = voe_game_models_update(world, models, device, FOLDER,
					  scratch, &progress);
	VOE_TEST_CHECK_INT(failures.count, 2);
	kept = voe_base_report_error_first();
	snprintf(path, sizeof(path), "%s: ", BROKEN[0]);
	VOE_TEST_CHECK(kept != NULL && strncmp(kept, path, strlen(path)) == 0);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// Every file and folder the case writes, deepest first.
static void remove_files(void)
{
	char path[256];

	copies_gone();
	for (uint32_t n = 0; n < 2; n++)
		remove(on_disk(path, sizeof(path), "", BROKEN[n], ""));
	for (uint32_t n = 0; n < FILES; n++)
		remove(on_disk(path, sizeof(path), "", PATHS[n], ""));
	remove(MODELS_FOLDER_ON_DISK);
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

	device =voe_render_device_new_headless(arena, size, CAPACITIES, &error);
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
	a_splash_cook_matches_one_by_one(device, arena, scratch);
	a_batchs_first_error_is_its_first_failing_file(device, arena, scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
