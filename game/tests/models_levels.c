// The model loader's levels against real files: the one-triangle `.glb` of
// glb_build.h and a bent grid built here, written into a scratch folder in
// the working directory (the build tree), and game worlds whose model rows
// name them.
//
// THE LEVELS, on a first world and store: an update leaves the `.glb`'s levels
// file in the folder's Cache, fitting the `.glb`; a prepare then makes none;
// the file made stale, a prepare makes it again, fitting; a remake on the
// fresh file makes it again, fitting; a missing `.glb` will not prepare. Then
// a bent 8 × 8 grid, `grid.glb`, prepared with remake: with no `grid.parts`
// three levels or more; with one saying `count=2`, two; with `count=9`, which
// will not parse, three or more again; with `swap="far"`, a file whose swaps 1
// to 5 are 2; with `swap2=3` alone, swap 2 is 3 and the others 1. On a second
// world and store, the grid updated holds three levels or more; a prepare to
// `count=2` leaves the store alone; a reload holds two; a reload of a path the
// store lacks adds nothing.
//
// THE SWAPS PATCHED: the grid prepared with the defaults, then `swap1=4`
// prepared without remake makes only the cooked copy, the levels file on disk
// at swap 1 4 with the same index counts; on a third world and store, the grid updated at ×4, then
// `swap1=0.5` reloaded, holds a level 1 bound 8 times what it was, capped at 1
// (0500 point 5).
//
// THE SUMMARY: the grid prepared with the defaults summarises its file's
// levels, made and fresh, level 0 its 128 triangles and each level fewer,
// level 1's error and the radius above nought; a missing `.glb` counts none.
//
// THE REFUSED MAKE: `bad.glb`, bytes no `.glb` has, named by a row and
// updated, is failed at the file's stamp, no levels file is in the Cache, and
// the kept first error, cleared before, begins with `bad.glb: `.
//
// The files and the folder are removed at the end, pass or fail. It skips
// when there is no graphics card, because a load uploads.
#include "glb_build.h"

#include <game/models.h>
#include <game/world.h>

#include <3d/landscape.h>
#include <3d/model_levels.h>
#include <3d/models.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FOLDER "game_models_levels_test"
#define GOOD "good.glb"
#define MISSING "missing.glb"
#define GOOD_ON_DISK FOLDER "/" GOOD
#define CACHE_ON_DISK FOLDER "/Cache"
#define LEVELS_FOLDER_ON_DISK CACHE_ON_DISK "/levels"
#define LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" GOOD ".levels"
#define GRID "grid.glb"
#define GRID_ON_DISK FOLDER "/" GRID
#define GRID_PARTS_ON_DISK FOLDER "/grid.parts"
#define GRID_LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" GRID ".levels"
#define BAD "bad.glb"
#define BAD_ON_DISK FOLDER "/" BAD
#define BAD_LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" BAD ".levels"
#define COPIES_FOLDER_ON_DISK CACHE_ON_DISK "/models"

// Room for the triangle twice over a reload, the pictures' quad, and the
// shadings of the triangle, the picture and the dot, plus two blended twins:
// one for each triangle part held at once over the reload (ADR-0336 point 2).
// Then a landscape as the store makes it (0396 point 3): the shared grid of
// VOE_3D_LANDSCAPE_NODE_QUADS² quads, its ground and its twin, and the
// heights texture, which takes a texture slot and no capacity. Then a
// material's record and twin, with no geometry, and a second one's. The
// swaps case's store alone holds the most geometries: the grid's levels, up
// to VOE_3D_MODEL_LEVELS, and as many it reloads to, at once.
#define GRID_SIDE (VOE_3D_LANDSCAPE_NODE_QUADS + 1)
static const voe_render_capacities CAPACITIES = {
	.vertices = 16 + GRID_SIDE * GRID_SIDE,
	.indices = 16 + VOE_3D_LANDSCAPE_NODE_QUADS *
				VOE_3D_LANDSCAPE_NODE_QUADS * 6,
	.geometries = VOE_3D_MODEL_LEVELS * 2,
	.objects = 1,
	.shadings = 10 + 2 + 2 + 2,
	.passes = 1,
};

// A grid of GRID_QUADS × GRID_QUADS unit quads bent both ways, z = (x² + y²)
// ÷ 128 from 0 to 1, so its levels have an error above nought and a finite
// bound: 81
// positions, then 384 32-bit indices in their own buffer view.
#define GRID_QUADS 8
#define GRID_POINTS ((GRID_QUADS + 1) * (GRID_QUADS + 1))
#define GRID_INDICES (GRID_QUADS * GRID_QUADS * 6)
static const char GRID_JSON[] =
	"{\"asset\":{\"version\":\"2.0\"},\"nodes\":[{\"mesh\":0}],"
	"\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},"
	"\"indices\":1}]}],"
	"\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":81,"
	"\"type\":\"VEC3\",\"min\":[0,0,0],\"max\":[8,8,1]},"
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
		bin.positions[i * 3 + 2] =
			(bin.positions[i * 3] * bin.positions[i * 3] +
			 bin.positions[i * 3 + 1] * bin.positions[i * 3 + 1]) /
			128.0f;
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

// The grid's levels file on disk read against `grid`: its counts and swaps,
// the indices rewound away; zeroed when it will not read.
static voe_3d_model_levels grid_read(voe_base_arena *scratch,
				     const uint8_t *grid, uint32_t size)
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
	return fits ? levels : (voe_3d_model_levels){ 0 };
}

// The grid prepared, remade, with `parts` as its parts file (NULL for none):
// true and made, its level count returned; 0 when it would not prepare.
static uint32_t grid_prepared(voe_render_device *device,
			      voe_base_arena *scratch, const uint8_t *grid,
			      uint32_t size, const char *parts)
{
	voe_base_error error = VOE_BASE_OK;
	bool made = false;
	bool ready;

	if (parts != NULL)
		VOE_TEST_CHECK(voe_platform_file_write(
			GRID_PARTS_ON_DISK, (const uint8_t *)parts,
			strlen(parts), NULL));
	ready = voe_game_models_prepare(FOLDER, GRID, device, scratch, true,
					&made, &error);
	VOE_TEST_CHECK(ready);
	VOE_TEST_CHECK(made);
	return ready ? grid_read(scratch, grid, size).level_count : 0;
}

// The grid's levels as its parts file says: the defaults with none, its
// count, and the defaults again for a count past the limit.
static void check_levels_asked(voe_render_device *device,
			       voe_base_arena *scratch)
{
	static uint8_t grid[4096];
	uint32_t size = grid_build(grid, sizeof(grid));

	VOE_TEST_CHECK(voe_platform_file_write(GRID_ON_DISK, grid, size, NULL));

	// ---- no parts file: the default count, 3 or more
	remove(GRID_PARTS_ON_DISK);
	VOE_TEST_CHECK(grid_prepared(device, scratch, grid, size, NULL) >= 3);

	// ---- `count=2`: two levels
	VOE_TEST_CHECK_INT(grid_prepared(device, scratch, grid, size,
					 "[Parts]\n[Levels]\ncount=2\n"),
			   2);

	// ---- `count=9` will not parse: the default again
	VOE_TEST_CHECK(grid_prepared(device, scratch, grid, size,
				     "[Parts]\n[Levels]\ncount=9\n") >= 3);

	// ---- `swap="far"`: the file's swaps 1 to 5 are 2
	VOE_TEST_CHECK(grid_prepared(device, scratch, grid, size,
				     "[Parts]\n[Levels]\nswap=\"far\"\n") >= 3);
	for (uint32_t k = 1; k < VOE_3D_MODEL_LEVELS; k++)
		VOE_TEST_CHECK(grid_read(scratch, grid, size).swaps[k] == 2.0f);

	// ---- `swap2=3` alone: swap 2 is 3, the others 1
	VOE_TEST_CHECK(grid_prepared(device, scratch, grid, size,
				     "[Parts]\n[Levels]\nswap2=3\n") >= 3);
	for (uint32_t k = 1; k < VOE_3D_MODEL_LEVELS; k++)
		VOE_TEST_CHECK(grid_read(scratch, grid, size).swaps[k] ==
			       (k == 2 ? 3.0f : 1.0f));
}

// The parts file's grid text written as `parts`.
static void grid_parts_write(const char *parts)
{
	VOE_TEST_CHECK(voe_platform_file_write(GRID_PARTS_ON_DISK,
					       (const uint8_t *)parts,
					       strlen(parts), NULL));
}

// The grid's entry's level 1 bound, 0 when the store lacks it or it has one
// level.
static float grid_bound(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GRID);

	return entry != NULL && entry->level_count > 1 ? entry->levels[1].bound :
							 0.0f;
}

// A swap-only change in the parts file patches the fresh levels file, made
// none, and a reload takes the new swap.
static void check_swaps_patched(voe_ecs_world *world,
				voe_render_device *device,
				voe_base_arena *scratch)
{
	static uint8_t grid[4096];
	uint32_t size = grid_build(grid, sizeof(grid));
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;
	voe_base_error error = VOE_BASE_OK;
	voe_3d_model_levels before;
	voe_3d_model_levels after;
	bool made = true;
	float bound;

	VOE_TEST_CHECK(voe_platform_file_write(GRID_ON_DISK, grid, size, NULL));

	// ---- the defaults, then `swap1=4` prepared without remake: the levels
	// patched, not made; the cooked copy, keyed by the new `.parts`' stamp,
	// is made again, so `made` is true
	remove(GRID_PARTS_ON_DISK);
	VOE_TEST_CHECK(grid_prepared(device, scratch, grid, size, NULL) >= 3);
	before = grid_read(scratch, grid, size);
	grid_parts_write("[Parts]\n[Levels]\nswap1=4\n");
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GRID, device, scratch,
					       false, &made, &error));
	VOE_TEST_CHECK(made);
	after = grid_read(scratch, grid, size);
	VOE_TEST_CHECK(after.swaps[1] == 4.0f);
	VOE_TEST_CHECK_INT(after.level_count, before.level_count);
	VOE_TEST_CHECK(memcmp(after.index_counts, before.index_counts,
			      sizeof(before.index_counts)) == 0);

	// ---- updated at ×4, then `swap1=0.5` reloaded: the bound 8 times, capped
	// at 1 (0500 point 5); level 1 follows only level 0's infinite bound, so
	// no spread moves it, and below 1 at ×4 it was not capped then
	wear(world, GRID);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	bound = grid_bound(models);
	VOE_TEST_CHECK(bound > 0.0f && bound < 1.0f);
	grid_parts_write("[Parts]\n[Levels]\nswap1=0.5\n");
	VOE_TEST_CHECK(voe_game_models_reload(models, device, FOLDER, GRID,
					      scratch, &error));
	VOE_TEST_CHECK(fabsf(grid_bound(models) - fminf(bound * 8.0f, 1.0f)) <=
		       fminf(bound * 8.0f, 1.0f) * 1e-5f);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// The grid's levels summarised as its fresh file has them; a missing `.glb`,
// none.
static void check_summary(voe_render_device *device, voe_base_arena *scratch)
{
	static uint8_t grid[4096];
	uint32_t size = grid_build(grid, sizeof(grid));
	voe_3d_model_levels_summary summary = { .own = true };

	VOE_TEST_CHECK(voe_platform_file_write(GRID_ON_DISK, grid, size, NULL));
	remove(GRID_PARTS_ON_DISK);
	VOE_TEST_CHECK(grid_prepared(device, scratch, grid, size, NULL) >= 3);

	// ---- the file's level count, level 0 whole, each level fewer, level
	// 1's error and the radius above nought
	voe_game_models_levels_summarise(FOLDER, GRID, scratch, &summary);
	VOE_TEST_CHECK_INT(summary.count,
			   grid_read(scratch, grid, size).level_count);
	VOE_TEST_CHECK(!summary.own);
	VOE_TEST_CHECK(summary.fresh);
	VOE_TEST_CHECK_INT(summary.triangles[0], GRID_INDICES / 3);
	for (uint32_t k = 1; k < summary.count; k++)
		VOE_TEST_CHECK(summary.triangles[k] < summary.triangles[k - 1]);
	VOE_TEST_CHECK(summary.errors[1] > 0.0f);
	VOE_TEST_CHECK(summary.radius > 0.0f);

	// ---- a missing `.glb`: none
	voe_game_models_levels_summarise(FOLDER, MISSING, scratch, &summary);
	VOE_TEST_CHECK_INT(summary.count, 0);
}

// A `.glb` its levels make refuses is kept failed at its stamp, with no
// levels file, and its kept error begins with its path.
static void check_refused_make(voe_ecs_world *world,
			       voe_render_device *device,
			       voe_base_arena *scratch)
{
	static const uint8_t BROKEN[] = "these bytes are no glb";
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;
	const voe_3d_model_entry *entry;
	const char *kept;
	uint64_t stamp = 0;

	VOE_TEST_CHECK(voe_platform_file_write(BAD_ON_DISK, BROKEN,
					       sizeof(BROKEN) - 1, NULL));
	VOE_TEST_CHECK(voe_platform_file_stamp(BAD_ON_DISK, &stamp));

	// ---- updated: failed at its stamp, no levels file, its path first
	wear(world, BAD);
	voe_base_report_error_clear();
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 1);
	entry = voe_3d_models_find(models, BAD);
	VOE_TEST_CHECK(entry != NULL);
	if (entry != NULL) {
		VOE_TEST_CHECK(!entry->loaded);
		VOE_TEST_CHECK(entry->stamp == stamp);
	}
	VOE_TEST_CHECK(!voe_platform_file_exists(BAD_LEVELS_ON_DISK));
	kept = voe_base_report_error_first();
	VOE_TEST_CHECK(kept != NULL &&
		       strncmp(kept, BAD ": ", sizeof(BAD ": ") - 1) == 0);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
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
	VOE_TEST_CHECK_INT(grid_prepared(device, scratch, grid, size,
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
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, device, scratch,
					       false, &made, &error));
	VOE_TEST_CHECK(!made);

	// ---- the file stale: made again, fitting
	VOE_TEST_CHECK(voe_platform_file_write(LEVELS_ON_DISK, STALE,
					       sizeof(STALE) - 1, NULL));
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, device, scratch,
					       false, &made, &error));
	VOE_TEST_CHECK(made);
	VOE_TEST_CHECK(levels_fit(scratch, glb, size));

	// ---- a remake on the fresh file: made again, fitting
	made = false;
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, device, scratch,
					       true, &made, &error));
	VOE_TEST_CHECK(made);
	VOE_TEST_CHECK(levels_fit(scratch, glb, size));

	// ---- a missing `.glb`: false
	VOE_TEST_CHECK(!voe_game_models_prepare(FOLDER, MISSING, device,
						scratch, false, &made, &error));

	check_levels_asked(device, scratch);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(GOOD_ON_DISK);
	remove(LEVELS_ON_DISK);
	remove(GRID_ON_DISK);
	remove(GRID_PARTS_ON_DISK);
	remove(GRID_LEVELS_ON_DISK);
	remove(BAD_ON_DISK);
	remove(BAD_LEVELS_ON_DISK);
	remove(LEVELS_FOLDER_ON_DISK);
	remove(COPIES_FOLDER_ON_DISK "/" GOOD ".model");
	remove(COPIES_FOLDER_ON_DISK "/" GRID ".model");
	remove(COPIES_FOLDER_ON_DISK "/" BAD ".model");
	remove(COPIES_FOLDER_ON_DISK);
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
	check_levels(voe_game_world_new(arena), device, scratch, glb, length);
	check_reload(voe_game_world_new(arena), device, scratch);
	check_swaps_patched(voe_game_world_new(arena), device, scratch);
	check_summary(device, scratch);
	check_refused_make(voe_game_world_new(arena), device, scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
