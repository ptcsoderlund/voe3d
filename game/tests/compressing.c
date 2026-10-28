// The compressing queue of game/compressing.h on a headless device, against
// PNGs it writes (voe_assets_png_encode) into a scratch folder in the working
// directory (the build tree). Each case steps its queue with a millisecond's
// sleep between steps, up to a bound, since a step never waits.
//
// a_job_ends_in_its_copy: a 64×64 colour PNG added and stepped until nothing
// waits; its path came back once, and its copy reads back BC7 at the
// picture's sides with its whole chain, keyed by the stamp.
// a_second_add_replaces: the same path and kind added at 21, stepped once so
// its read runs, added again at 22: one waits, the path comes back once and
// the copy is keyed 22.
// clear_drops_every_job: two paths added and the first stepped a few times,
// two wait; cleared, none, and no path comes back.
// a_missing_map_writes_nothing: a path with no file added, stepped until none
// waits: no path came back and no copy is there.
// a_normal_map_is_bc5: the PNG added as a normal map, its copy BC5.
//
// The files and the folders are removed at the end, pass or fail. It skips
// when there is no graphics card, because the GPU compresses.
#include "../src/models_cooked.h"

#include <game/compressing.h>

#include <assets/image.h>

#include <base/arena.h>
#include <base/error.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#define FOLDER "game_compressing_test"
#define PICTURE "pic.png"
#define OTHER "other.png"
#define MISSING "missing.png"
#define SIDE 64
#define PICTURE_ON_DISK FOLDER "/" PICTURE
#define OTHER_ON_DISK FOLDER "/" OTHER
#define CACHE_ON_DISK FOLDER "/Cache"
#define PICTURES_FOLDER_ON_DISK CACHE_ON_DISK "/pictures"
#define COPY_ON_DISK PICTURES_FOLDER_ON_DISK "/" PICTURE ".colour"
#define NORMAL_ON_DISK PICTURES_FOLDER_ON_DISK "/" PICTURE ".normal"
#define OTHER_COPY_ON_DISK PICTURES_FOLDER_ON_DISK "/" OTHER ".colour"
#define MISSING_COPY_ON_DISK PICTURES_FOLDER_ON_DISK "/" MISSING ".colour"

// Steps of a millisecond at most before a case gives up: ten seconds.
#define STEPS_AT_MOST 10000

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 4,
	.passes = 1,
};

// A 64×64 gradient written as the PNG `file`; its stamp. Scratch rewound.
static uint64_t png_write(const char *file, voe_base_arena *scratch)
{
	static uint8_t pixels[SIDE * SIDE * 4];
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_assets_image image = { SIDE, SIDE, pixels, 0,
				   VOE_ASSETS_IMAGE_RGBA8 };
	voe_assets_bytes png = { 0 };
	uint64_t stamp = 0;

	for (uint32_t i = 0; i < sizeof(pixels); i++)
		pixels[i] = (uint8_t)(i % 4 == 3 ? 255 : i * 7);
	VOE_TEST_CHECK(voe_assets_png_encode(scratch, image, &png, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(file, png.bytes, png.count,
					       NULL));
	VOE_TEST_CHECK(voe_platform_file_stamp(file, &stamp));
	voe_base_arena_rewind(scratch, mark);
	return stamp;
}

// The queue stepped, a millisecond between steps, until none waits or the
// bound; how many steps handed `path` back. Scratch rewound.
static uint32_t steps_to_end(voe_game_compressing *queue,
			     voe_render_device *device, const char *path,
			     voe_base_arena *scratch)
{
	const struct timespec millisecond = { .tv_nsec = 1000000 };
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	uint32_t ended = 0;

	for (uint32_t i = 0;
	     i < STEPS_AT_MOST && voe_game_compressing_waiting(queue) > 0;
	     i++) {
		const char *written =
			voe_game_compressing_step(queue, device, scratch);

		ended += written != NULL && strcmp(written, path) == 0;
		(void)thrd_sleep(&millisecond, NULL);
	}
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 0);
	voe_base_arena_rewind(scratch, mark);
	return ended;
}

// The copy of `kind` reads back a full chain in `format` at the picture's
// sides, keyed `stamp`. Scratch rewound.
static void copy_check(voe_assets_image_kind kind,
		       voe_assets_image_format format, uint64_t stamp,
		       voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_assets_image image = { 0 };

	VOE_TEST_CHECK(voe_game_models_picture_take(FOLDER, PICTURE, kind,
						    stamp, true, scratch,
						    &image));
	VOE_TEST_CHECK_INT(image.format, format);
	VOE_TEST_CHECK_INT(image.width, SIDE);
	VOE_TEST_CHECK_INT(image.height, SIDE);
	VOE_TEST_CHECK_INT(image.levels,
			   voe_assets_image_level_count(SIDE, SIDE));
	voe_base_arena_rewind(scratch, mark);
}

static void a_job_ends_in_its_copy(voe_render_device *device, uint64_t stamp,
				   voe_base_arena *scratch)
{
	voe_game_compressing *queue = voe_game_compressing_new();

	remove(COPY_ON_DISK);
	voe_game_compressing_add(queue, FOLDER, PICTURE,
				 VOE_ASSETS_IMAGE_COLOUR, stamp);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 1);
	VOE_TEST_CHECK_INT(steps_to_end(queue, device, PICTURE, scratch), 1);
	copy_check(VOE_ASSETS_IMAGE_COLOUR, VOE_ASSETS_IMAGE_BC7, stamp,
		   scratch);
	voe_game_compressing_destroy(queue, device);
}

static void a_second_add_replaces(voe_render_device *device,
				  voe_base_arena *scratch)
{
	voe_game_compressing *queue = voe_game_compressing_new();

	remove(COPY_ON_DISK);
	voe_game_compressing_add(queue, FOLDER, PICTURE,
				 VOE_ASSETS_IMAGE_COLOUR, 21);
	VOE_TEST_CHECK(voe_game_compressing_step(queue, device, scratch) ==
		       NULL);
	voe_game_compressing_add(queue, FOLDER, PICTURE,
				 VOE_ASSETS_IMAGE_COLOUR, 22);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 1);
	VOE_TEST_CHECK_INT(steps_to_end(queue, device, PICTURE, scratch), 1);
	copy_check(VOE_ASSETS_IMAGE_COLOUR, VOE_ASSETS_IMAGE_BC7, 22, scratch);
	voe_game_compressing_destroy(queue, device);
}

static void clear_drops_every_job(voe_render_device *device, uint64_t stamp,
				  voe_base_arena *scratch)
{
	voe_game_compressing *queue = voe_game_compressing_new();
	uint32_t ended = 0;

	remove(OTHER_COPY_ON_DISK);
	voe_game_compressing_add(queue, FOLDER, OTHER, VOE_ASSETS_IMAGE_COLOUR,
				 stamp);
	voe_game_compressing_add(queue, FOLDER, PICTURE,
				 VOE_ASSETS_IMAGE_DATA, stamp);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 2);
	// Its read started and taken, its GPU job started: a running stage.
	for (uint32_t i = 0; i < 3; i++)
		ended += voe_game_compressing_step(queue, device, scratch) !=
			 NULL;
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 2);
	voe_game_compressing_clear(queue, device);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 0);
	ended += voe_game_compressing_step(queue, device, scratch) != NULL;
	VOE_TEST_CHECK_INT(ended, 0);
	VOE_TEST_CHECK(!voe_platform_file_exists(OTHER_COPY_ON_DISK));
	voe_game_compressing_destroy(queue, device);
}

static void a_missing_map_writes_nothing(voe_render_device *device,
					 voe_base_arena *scratch)
{
	voe_game_compressing *queue = voe_game_compressing_new();

	voe_game_compressing_add(queue, FOLDER, MISSING,
				 VOE_ASSETS_IMAGE_COLOUR, 41);
	VOE_TEST_CHECK_INT(steps_to_end(queue, device, MISSING, scratch), 0);
	VOE_TEST_CHECK(!voe_platform_file_exists(MISSING_COPY_ON_DISK));
	voe_game_compressing_destroy(queue, device);
}

static void a_normal_map_is_bc5(voe_render_device *device, uint64_t stamp,
				voe_base_arena *scratch)
{
	voe_game_compressing *queue = voe_game_compressing_new();

	remove(NORMAL_ON_DISK);
	voe_game_compressing_add(queue, FOLDER, PICTURE,
				 VOE_ASSETS_IMAGE_NORMAL, stamp);
	VOE_TEST_CHECK_INT(steps_to_end(queue, device, PICTURE, scratch), 1);
	copy_check(VOE_ASSETS_IMAGE_NORMAL, VOE_ASSETS_IMAGE_BC5, stamp,
		   scratch);
	voe_game_compressing_destroy(queue, device);
}

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(COPY_ON_DISK);
	remove(NORMAL_ON_DISK);
	remove(OTHER_COPY_ON_DISK);
	remove(MISSING_COPY_ON_DISK);
	remove(PICTURES_FOLDER_ON_DISK "/" PICTURE ".data");
	remove(PICTURES_FOLDER_ON_DISK);
	remove(CACHE_ON_DISK);
	remove(PICTURE_ON_DISK);
	remove(OTHER_ON_DISK);
	remove(FOLDER);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_arena *scratch = voe_base_arena_new(1 << 16);
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	uint64_t stamp;

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
	stamp = png_write(PICTURE_ON_DISK, scratch);
	(void)png_write(OTHER_ON_DISK, scratch);
	a_job_ends_in_its_copy(device, stamp, scratch);
	a_second_add_replaces(device, scratch);
	clear_drops_every_job(device, stamp, scratch);
	a_missing_map_writes_nothing(device, scratch);
	a_normal_map_is_bc5(device, stamp, scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
