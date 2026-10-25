// The compressing queue of game/compressing.h against a scratch folder in the
// working directory (the build tree), made as models_cooked.c makes its own.
//
// a_step_writes_the_copy: a 64×64 colour chain added and stepped with 0
// seconds until a path comes back, more than one step since a step encodes
// at least one run of 64 of its 343 blocks; its copy reads back BC7 through
// voe_game_models_picture_take, keyed by the stamp, and none waits.
// a_second_add_replaces_the_first: the same path and kind added at two
// stamps, one waits; the copy written is keyed by the second.
// clear_leaves_none_waiting: two paths added, two wait; cleared, none.
//
// The files and the folders are removed at the end, pass or fail. No graphics
// card is needed: nothing uploads.
#include "../src/models_cooked.h"

#include <game/compressing.h>

#include <assets/image.h>

#include <base/arena.h>

#include <platform/folder.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define FOLDER "game_compressing_test"
#define PICTURE "pic.png"
#define OTHER "other.png"
#define SIDE 64
#define CACHE_ON_DISK FOLDER "/Cache"
#define PICTURES_FOLDER_ON_DISK CACHE_ON_DISK "/pictures"
#define COPY_ON_DISK PICTURES_FOLDER_ON_DISK "/" PICTURE ".colour"

// Steps a run of 64 blocks at least; a 64×64 chain has 343.
#define STEPS_AT_MOST 16

// A 64×64 gradient's full colour chain on `arena`.
static voe_assets_image chain_made(voe_base_arena *arena)
{
	static uint8_t pixels[SIDE * SIDE * 4];
	voe_assets_image image = { SIDE, SIDE, pixels, 0,
				   VOE_ASSETS_IMAGE_RGBA8 };

	for (uint32_t i = 0; i < sizeof(pixels); i++)
		pixels[i] = (uint8_t)(i % 4 == 3 ? 255 : i * 7);
	return voe_assets_image_mips(arena, &image, VOE_ASSETS_IMAGE_COLOUR);
}

// The queue stepped with 0 seconds until a path comes back; the steps taken.
static uint32_t steps_to_end(voe_game_compressing *queue,
			     voe_base_arena *scratch)
{
	const char *ended = NULL;
	uint32_t steps = 0;

	while (ended == NULL && steps < STEPS_AT_MOST) {
		ended = voe_game_compressing_step(queue, 0.0, scratch);
		steps++;
	}
	VOE_TEST_CHECK(ended != NULL && strcmp(ended, PICTURE) == 0);
	return steps;
}

// The copy reads back a BC7 colour chain keyed `stamp`. Scratch rewound.
static void copy_check(uint64_t stamp, voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_assets_image image = { 0 };

	VOE_TEST_CHECK(voe_game_models_picture_take(FOLDER, PICTURE,
						    VOE_ASSETS_IMAGE_COLOUR,
						    stamp, true, scratch,
						    &image));
	VOE_TEST_CHECK_INT(image.format, VOE_ASSETS_IMAGE_BC7);
	VOE_TEST_CHECK_INT(image.width, SIDE);
	VOE_TEST_CHECK_INT(image.levels,
			   voe_assets_image_level_count(SIDE, SIDE));
	voe_base_arena_rewind(scratch, mark);
}

static void a_step_writes_the_copy(const voe_assets_image *chain,
				   voe_base_arena *scratch)
{
	voe_game_compressing *queue = voe_game_compressing_new();

	remove(COPY_ON_DISK);
	voe_game_compressing_add(queue, FOLDER, PICTURE,
				 VOE_ASSETS_IMAGE_COLOUR, 11, chain);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 1);
	VOE_TEST_CHECK(steps_to_end(queue, scratch) > 1);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 0);
	copy_check(11, scratch);
	voe_game_compressing_destroy(queue);
}

static void a_second_add_replaces_the_first(const voe_assets_image *chain,
					    voe_base_arena *scratch)
{
	voe_game_compressing *queue = voe_game_compressing_new();

	remove(COPY_ON_DISK);
	voe_game_compressing_add(queue, FOLDER, PICTURE,
				 VOE_ASSETS_IMAGE_COLOUR, 21, chain);
	voe_game_compressing_add(queue, FOLDER, PICTURE,
				 VOE_ASSETS_IMAGE_COLOUR, 22, chain);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 1);
	(void)steps_to_end(queue, scratch);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 0);
	copy_check(22, scratch);
	voe_game_compressing_destroy(queue);
}

static void clear_leaves_none_waiting(const voe_assets_image *chain)
{
	voe_game_compressing *queue = voe_game_compressing_new();

	voe_game_compressing_add(queue, FOLDER, PICTURE,
				 VOE_ASSETS_IMAGE_COLOUR, 31, chain);
	voe_game_compressing_add(queue, FOLDER, OTHER, VOE_ASSETS_IMAGE_COLOUR,
				 31, chain);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 2);
	voe_game_compressing_clear(queue);
	VOE_TEST_CHECK_INT(voe_game_compressing_waiting(queue), 0);
	voe_game_compressing_destroy(queue);
}

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(COPY_ON_DISK);
	remove(PICTURES_FOLDER_ON_DISK);
	remove(CACHE_ON_DISK);
	remove(FOLDER);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 16);
	voe_base_arena *scratch = voe_base_arena_new(1 << 16);
	voe_assets_image chain;

	// A folder left by a run that crashed goes first.
	remove_files();
	VOE_TEST_CHECK(voe_platform_folder_create(FOLDER, NULL));
	chain = chain_made(arena);
	a_step_writes_the_copy(&chain, scratch);
	a_second_add_replaces_the_first(&chain, scratch);
	clear_leaves_none_waiting(&chain);
	remove_files();
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
