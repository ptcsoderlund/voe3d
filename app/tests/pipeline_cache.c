// The pipeline cache kept in a file (app/include/app/pipeline_cache.h). Device
// A prepares and saves to a path whose two folders are not there yet; device B
// loads that file and prepares; device C loads a file of garbage, says so on
// stderr and still prepares; a NULL path and a path with no file load and save
// nothing. The path call, when this machine has a settings folder, ends in
// `voe3d/<name>`.
//
// The files are written under the working directory ctest runs the test in, as
// start_log.c writes its log, and removed at the end, pass or fail.
//
// Headless, every device of its own so each starts unprepared. A machine with
// no usable Vulkan skips and says so.
#include <app/pipeline_cache.h>

#include <base/arena.h>
#include <base/error.h>
#include <platform/file.h>
#include <platform/path.h>

#include <testing/test.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ROOT "app_pipeline_cache_test"
#define FOLDER ROOT "/voe3d"
#define CACHE_PATH FOLDER "/pipelines_test.cache"
#define GARBAGE_PATH FOLDER "/garbage.cache"
#define MISSING_PATH FOLDER "/no_such.cache"

// Enough for a device to open; nothing is drawn.
static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.elements = 1,
	.passes = 1,
};

// A headless device, or NULL; `skipped` says whether that was no Vulkan here.
static voe_render_device *open_device(voe_base_arena *arena, bool *skipped)
{
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);

	*skipped = device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				      error == VOE_BASE_ERROR_UNSUPPORTED);
	if (*skipped)
		printf("skip: %s\n", voe_base_error_string(error));
	else
		VOE_TEST_CHECK(device != NULL);
	return device;
}

// Every prepare step taken; what the last one answered.
static voe_render_prepare prepare_all(voe_render_device *device)
{
	uint32_t steps = voe_render_device_prepare_steps(device);
	voe_render_prepare answer = VOE_RENDER_PREPARING;

	for (uint32_t i = 0; i <= steps && answer == VOE_RENDER_PREPARING; i++)
		answer = voe_render_device_prepare(device);
	return answer;
}

static void path_ends_in_voe3d(voe_base_arena *arena)
{
	const char *path = voe_app_pipeline_cache_path(arena, "pipelines_x.cache");
	const char *parent;

	if (path == NULL) {
		printf("no settings folder here; the path is NULL\n");
		return;
	}
	parent = voe_platform_path_parent(arena, path);
	VOE_TEST_CHECK(strcmp(voe_platform_path_name(path),
			      "pipelines_x.cache") == 0);
	VOE_TEST_CHECK(parent != NULL &&
		       strcmp(voe_platform_path_name(parent), "voe3d") == 0);
}

int main(void)
{
	static const char GARBAGE[] = "not a pipeline cache at all";
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_render_device *device;
	const uint8_t *saved;
	size_t size = 0;
	bool skipped = false;

	path_ends_in_voe3d(arena);

	// A: prepares and saves; the folders are made on the way.
	device = open_device(arena, &skipped);
	if (device == NULL)
		goto out;
	voe_app_pipeline_cache_load(device, NULL, arena);
	voe_app_pipeline_cache_load(device, MISSING_PATH, arena);
	VOE_TEST_CHECK_INT(prepare_all(device), VOE_RENDER_PREPARED);
	voe_app_pipeline_cache_save(device, CACHE_PATH, arena);
	voe_app_pipeline_cache_save(device, NULL, arena);
	voe_render_device_destroy(device);
	saved = voe_platform_file_read(CACHE_PATH, arena, &size, NULL);
	VOE_TEST_CHECK(saved != NULL && size > 0);
	VOE_TEST_CHECK(!voe_platform_file_exists(MISSING_PATH));

	// B: loads that file and prepares on it.
	device = open_device(arena, &skipped);
	if (device == NULL)
		goto out;
	voe_app_pipeline_cache_load(device, CACHE_PATH, arena);
	VOE_TEST_CHECK_INT(prepare_all(device), VOE_RENDER_PREPARED);
	voe_render_device_destroy(device);

	// C: a file of garbage is refused and costs nothing but a line.
	VOE_TEST_CHECK(voe_platform_file_write(GARBAGE_PATH,
					       (const uint8_t *)GARBAGE,
					       sizeof(GARBAGE) - 1, NULL));
	device = open_device(arena, &skipped);
	if (device == NULL)
		goto out;
	voe_app_pipeline_cache_load(device, GARBAGE_PATH, arena);
	VOE_TEST_CHECK_INT(prepare_all(device), VOE_RENDER_PREPARED);
	voe_render_device_destroy(device);

out:
	remove(CACHE_PATH);
	remove(GARBAGE_PATH);
	remove(FOLDER);
	remove(ROOT);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
