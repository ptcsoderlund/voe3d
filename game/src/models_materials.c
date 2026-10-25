// The material loader of game/include/game/models.h: every loaded model entry
// given its parts from the parts table; then every material path a shape
// names, every model part's `wears` and loaded landscape's layer, that the
// store lacks, gathered: one the table has, once, one it lacks kept failed at
// 0 and counted, its line said here.
//
// THE PHASES (0448 points 3, 4, 0459 point 1), in order on the calling
// thread, one material at a time: reading (each map's cooked copy taken onto
// the map's own arena as the call's mode allows, else the map joined onto the
// folder and read onto the material's arena, under its path as the subject),
// decoding pictures (each map read, on its own arena, under its path and "the
// <kind> map `path`" as 3d says it), cooking (each decoded map mipped and
// written as its copy, compressed, unless the mode writes none; given a queue,
// mipped and added to it instead, game/compressing.h, 0477 point 3), uploading
// (through voe_3d_models_put_material, a copy's or a queued map's levels as
// they are, under its path). A
// map that will not read or decode keeps the material failed at its stamp,
// the cause said where it was met (0445); every failure counted. Each
// material's arenas are destroyed after its put; `scratch` holds only the
// call's lists. The one-material load goes the same way.
//
// THE MODES (game/src/models_cooked.h, 0470 points 2, 4, 5): the materials
// call takes a fresh copy and writes a missing or stale one; the one-material
// load takes a fresh one and writes none; materials_cooked, the game's,
// takes any copy that reads and writes one only when there is none that does.
//
// THE PROGRESS (0477 point 3): "Loading materials" done/total before each
// material is read, then "Compressing textures" at the same count once it is
// read when a map of it will be cooked inline.
//
// THE LINE, `game: materials: <n> files in <s> s: reading <s> s, decoding
// pictures <s> s, uploading <s> s, cooking <s> s`, the maps or copies read,
// once per call that read one (game/src/models_batch.h).
//
// A full store ends the gathering, so every gathered material has room.
//
// Constraints: a path is found in a table by comparing it with every entry: a
// scan per unread material path, once each since a failure is kept, and a scan
// of the parts table per model entry per call, each frame in the game; a
// gathered path is compared with every one gathered before it. Sorted tables
// would lift it. Two broken maps of one material each say their line.
#include "models_batch.h"

#include <game/models.h>

#include <3d/model_prepare.h>
#include <3d/shape_component.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/clock.h>
#include <platform/file.h>
#include <platform/path.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// A material's four maps, in the order the put uploads them.
#define MAPS 4
// Block sizes, not limits: a push past one gets a block of its own.
#define MATERIAL_BLOCK (1024 * 1024)
#define PICTURE_BLOCK (1024 * 1024)

static const char *const MAP_NAMES[MAPS] = { "colour", "normal", "ORM",
					     "emission" };
// Each map's kind by its slot (0470 point 2).
static const voe_assets_image_kind MAP_KINDS[MAPS] = {
	VOE_ASSETS_IMAGE_COLOUR, VOE_ASSETS_IMAGE_NORMAL, VOE_ASSETS_IMAGE_DATA,
	VOE_ASSETS_IMAGE_COLOUR
};

// One material of a chunk, from its read to its put.
struct chunk_material {
	const voe_game_material *material;
	// Its maps' bytes, NULL until read and after its put.
	voe_base_arena *arena;
	voe_3d_material_map maps[MAPS];
	// Every map read, or the category of the one that would not.
	bool read;
	voe_base_error error;
	// Each map's stamp, 0 for none, and whether its copy was taken.
	uint64_t stamps[MAPS];
	bool taken[MAPS];
	// Each map's picture on an arena of its own, written by its copy's take,
	// its decode or its cook.
	voe_base_arena *picture_arenas[MAPS];
	voe_assets_image pictures[MAPS];
	voe_base_error decoded[MAPS];
};

// A chunk of materials of `folder`: one, as each call loads them, taking and
// writing map copies as `cooking` says, or queueing maps in `compressing`
// when not NULL; `done` of `total` the call's count for `progress`.
struct chunk {
	const char *folder;
	struct chunk_material *materials;
	uint32_t count;
	enum voe_game_models_cooking cooking;
	voe_game_compressing *compressing;
	voe_game_progress *progress;
	uint32_t done;
	uint32_t total;
};

// The `index`th map's path of `values`, "" for none.
static const char *map_path(const voe_assets_material_file *values,
			    uint32_t index)
{
	const char *const paths[MAPS] = { values->colormap, values->normalmap,
					  values->ormmap, values->emissionmap };

	VOE_BASE_ASSERT(index < MAPS, "a map past a material's four");
	return paths[index];
}

// Reads `path` from `folder` into `map`; "" leaves it empty. False, with the
// category in `error`, when the file will not read.
static bool read_map(const char *folder, const char *path,
		     voe_base_arena *arena, voe_3d_material_map *map,
		     voe_base_error *error)
{
	VOE_BASE_ASSERT(path != NULL && map != NULL, "reading no map");
	*map = (voe_3d_material_map){ 0 };
	if (path[0] == '\0')
		return true;
	map->bytes = voe_platform_file_read(
		voe_platform_path_join(arena, folder, path), arena, &map->size,
		error);
	VOE_BASE_ASSERT(map->bytes != NULL || *error != VOE_BASE_OK,
			"an unread map with no category");
	return map->bytes != NULL;
}

// Map `k`'s stamp kept, then its copy taken onto the map's own arena, made
// here, as the chunk's mode allows: a copy whose key is the stamp, or any
// that reads in the game's. Whether one was taken.
static bool map_take(const struct chunk *chunk, struct chunk_material *read,
		     uint32_t k)
{
	const char *path = map_path(&read->material->values, k);
	bool compare = chunk->cooking != VOE_GAME_MODELS_COOK_MISSING;
	struct voe_base_arena_mark mark = voe_base_arena_mark(read->arena);

	VOE_BASE_ASSERT(read->picture_arenas[k] == NULL, "taking a map twice");
	if (path[0] == '\0')
		return false;
	// A map with no stamp has none to compare; its read says why.
	if (!voe_platform_file_stamp(
		    voe_platform_path_join(read->arena, chunk->folder, path),
		    &read->stamps[k]))
		read->stamps[k] = 0;
	voe_base_arena_rewind(read->arena, mark);
	read->picture_arenas[k] = voe_base_arena_new(PICTURE_BLOCK);
	if (compare && read->stamps[k] == 0)
		return false;
	read->taken[k] = voe_game_models_picture_take(
		chunk->folder, path, MAP_KINDS[k], read->stamps[k], compare,
		read->picture_arenas[k], &read->pictures[k]);
	return read->taken[k];
}

// Reading: a material's maps in order, each its copy or its bytes onto the
// material's arena, stopping at the first that will not read.
static void chunk_read(struct chunk *chunk, uint32_t index)
{
	struct chunk_material *read = &chunk->materials[index];
	const voe_assets_material_file *values = &read->material->values;

	VOE_BASE_ASSERT(index < chunk->count, "reading past the chunk");
	VOE_BASE_ASSERT(read->arena == NULL, "reading a material twice");
	voe_base_report_subject_push(read->material->path);
	read->arena = voe_base_arena_new(MATERIAL_BLOCK);
	read->read = true;
	for (uint32_t k = 0; read->read && k < MAPS; k++)
		read->read = map_take(chunk, read, k) ||
			     read_map(chunk->folder, map_path(values, k),
				      read->arena, &read->maps[k], &read->error);
	voe_base_report_subject_pop();
}

// Decoding pictures: one map of a read material, as 3d's load decodes it: a
// path that is no picture said here, a decoder's cause under the map.
static void chunk_decode(struct chunk *chunk, uint32_t index)
{
	struct chunk_material *read = &chunk->materials[index / MAPS];
	uint32_t k = index % MAPS;
	const char *path = map_path(&read->material->values, k);
	char subject[VOE_ASSETS_MATERIAL_PATH + 32];

	VOE_BASE_ASSERT(index < chunk->count * MAPS, "decoding past the chunk");
	if (!read->read || read->maps[k].size == 0)
		return;
	voe_base_report_subject_push(read->material->path);
	if (!voe_3d_model_picture_is(path)) {
		VOE_BASE_ERROR("game",
			       "the %s map `%s` is not a PNG or JPEG picture; save it as PNG",
			       MAP_NAMES[k], path);
		read->decoded[k] = VOE_BASE_ERROR_MALFORMED;
	} else {
		(void)snprintf(subject, sizeof(subject), "the %s map `%s`",
			       MAP_NAMES[k], path);
		voe_base_report_subject_push(subject);
		VOE_BASE_ASSERT(read->picture_arenas[k] != NULL,
				"decoding a map with no arena");
		if (!voe_3d_model_picture_decode(path, read->maps[k].bytes,
						 read->maps[k].size,
						 read->picture_arenas[k],
						 &read->pictures[k],
						 &read->decoded[k]))
			read->decoded[k] = VOE_BASE_ERROR_MALFORMED;
		voe_base_report_subject_pop();
	}
	voe_base_report_subject_pop();
}

// Whether the chunk's read maps will be cooked inline: one was read, not
// taken, with no queue and a mode that writes.
static bool chunk_cooks(const struct chunk *chunk)
{
	VOE_BASE_ASSERT(chunk != NULL, "no chunk");
	if (chunk->compressing != NULL ||
	    chunk->cooking == VOE_GAME_MODELS_COOK_NEVER)
		return false;
	for (uint32_t i = 0; i < chunk->count; i++)
		for (uint32_t k = 0; k < MAPS; k++)
			if (chunk->materials[i].read &&
			    chunk->materials[i].maps[k].size > 0)
				return true;
	return false;
}

// Cooking: one decoded map of a read material, its copy not taken, given its
// full chain on the CPU and added to the chunk's queue, or else written as its
// copy under its path unless the chunk's mode writes none.
static void chunk_cook(struct chunk *chunk, uint32_t index)
{
	struct chunk_material *read = &chunk->materials[index / MAPS];
	uint32_t k = index % MAPS;
	const char *path = map_path(&read->material->values, k);

	VOE_BASE_ASSERT(index < chunk->count * MAPS, "cooking past the chunk");
	if (!read->read || read->taken[k] || read->decoded[k] != VOE_BASE_OK ||
	    read->pictures[k].width == 0)
		return;
	if (chunk->compressing == NULL &&
	    chunk->cooking == VOE_GAME_MODELS_COOK_NEVER)
		return;
	VOE_BASE_ASSERT(read->picture_arenas[k] != NULL, "a map with no arena");
	voe_base_report_subject_push(read->material->path);
	if (chunk->compressing != NULL) {
		read->pictures[k] = voe_assets_image_mips(read->picture_arenas[k],
							  &read->pictures[k],
							  MAP_KINDS[k]);
		voe_game_compressing_add(chunk->compressing, chunk->folder, path,
					 MAP_KINDS[k], read->stamps[k],
					 &read->pictures[k]);
	} else {
		read->pictures[k] = voe_game_models_picture_save(
			chunk->folder, path, &read->pictures[k], MAP_KINDS[k],
			read->stamps[k], read->picture_arenas[k]);
	}
	voe_base_report_subject_pop();
}

// Reading, decoding pictures, then cooking, each timed, index by index in
// order; "Compressing textures" said once read when a map is cooked inline.
static void chunk_prepare(struct chunk *chunk, voe_game_models_times *times)
{
	double before = voe_platform_clock_now();

	VOE_BASE_ASSERT(chunk != NULL && times != NULL, "no chunk or times");
	for (uint32_t i = 0; i < chunk->count; i++)
		chunk_read(chunk, i);
	times->seconds[VOE_GAME_MODELS_READING] +=
		voe_platform_clock_now() - before;
	if (chunk_cooks(chunk))
		voe_game_progress_set(chunk->progress, "Compressing textures",
				      chunk->done, chunk->total);
	before = voe_platform_clock_now();
	for (uint32_t i = 0; i < chunk->count * MAPS; i++)
		chunk_decode(chunk, i);
	times->seconds[VOE_GAME_MODELS_DECODING] +=
		voe_platform_clock_now() - before;
	before = voe_platform_clock_now();
	for (uint32_t i = 0; i < chunk->count * MAPS; i++)
		chunk_cook(chunk, i);
	times->seconds[VOE_GAME_MODELS_COOKING] +=
		voe_platform_clock_now() - before;
	for (uint32_t i = 0; i < chunk->count; i++)
		for (uint32_t k = 0; k < MAPS; k++)
			times->files += chunk->materials[i].maps[k].bytes !=
						NULL ||
					chunk->materials[i].taken[k];
}

// The material's arenas given back.
static void material_release(struct chunk_material *read)
{
	for (uint32_t k = 0; k < MAPS; k++)
		if (read->picture_arenas[k] != NULL)
			voe_base_arena_destroy(read->picture_arenas[k]);
	if (read->arena != NULL)
		voe_base_arena_destroy(read->arena);
	*read = (struct chunk_material){ .material = read->material };
	VOE_BASE_ASSERT(read->arena == NULL, "a material's arena kept");
}

// Uploading: the material into the store at `stamp`, or kept failed there
// when a map would not read or decode, under its path; then its arenas given
// back. Whether it loaded, `error` its category.
static bool material_put(struct chunk_material *read, voe_3d_models *models,
			 voe_render_device *device, uint64_t stamp,
			 voe_base_error *error, voe_game_models_times *times)
{
	double before = voe_platform_clock_now();
	const char *path = read->material->path;
	bool loaded = false;

	*error = read->read ? VOE_BASE_OK : read->error;
	for (uint32_t k = 0; k < MAPS && *error == VOE_BASE_OK; k++)
		*error = read->decoded[k];
	voe_base_report_subject_push(path);
	if (*error != VOE_BASE_OK)
		voe_3d_models_fail(models, path, stamp);
	else
		loaded = voe_3d_models_put_material(
			models, device, path, stamp, &read->material->values,
			(voe_3d_material_pictures){
				read->pictures[0], read->pictures[1],
				read->pictures[2], read->pictures[3] },
			error);
	voe_base_report_subject_pop();
	material_release(read);
	times->seconds[VOE_GAME_MODELS_UPLOADING] +=
		voe_platform_clock_now() - before;
	VOE_BASE_ASSERT(loaded || *error != VOE_BASE_OK,
			"a failed material with no category");
	return loaded;
}

bool voe_game_models_material_load(voe_3d_models *models,
				   voe_render_device *device,
				   const char *folder,
				   const voe_game_material *material,
				   uint64_t stamp, voe_base_arena *scratch,
				   voe_game_compressing *compressing,
				   voe_base_error *error)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_game_models_times times = { 0 };
	struct chunk chunk = { .folder = folder,
			       .count = 1,
			       .cooking = VOE_GAME_MODELS_COOK_NEVER,
			       .compressing = compressing };
	bool loaded;

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(material != NULL && material->path[0] != '\0',
			"loading a material with no path");
	chunk.materials = voe_base_arena_push(scratch, sizeof(*chunk.materials));
	*chunk.materials = (struct chunk_material){ .material = material };
	chunk_prepare(&chunk, &times);
	loaded = material_put(chunk.materials, models, device, stamp, error,
			      &times);
	voe_game_models_times_write(&times, "materials",
				    VOE_GAME_MATERIALS_LINE);
	voe_base_arena_rewind(scratch, mark);
	return loaded;
}

// How many material paths `entry` names: its parts' `wears`, then a loaded
// landscape's layers; none when it failed.
static uint32_t entry_paths(const voe_3d_model_entry *entry)
{
	VOE_BASE_ASSERT(entry != NULL, "no entry");
	if (!entry->loaded)
		return 0;
	return entry->part_count +
	       (entry->landscape != NULL ? entry->landscape->layer_count : 0);
}

// `entry`'s `index`th material path, below entry_paths.
static const char *entry_path_at(const voe_3d_model_entry *entry,
				 uint32_t index)
{
	VOE_BASE_ASSERT(index < entry_paths(entry), "a path past the entry's");
	if (index < entry->part_count)
		return entry->parts[index].wears;
	return entry->landscape->layers[index - entry->part_count];
}

// `path`'s parts in `parts`, NULL when it has none.
static const voe_assets_parts *find_parts(const voe_game_parts *parts,
					  const char *path)
{
	VOE_BASE_ASSERT(parts != NULL && path != NULL, "no parts or path");
	for (uint32_t i = 0; i < parts->count; i++)
		if (strcmp(parts->models[i].path, path) == 0)
			return &parts->models[i].parts;
	return NULL;
}

// Every loaded model entry, no picture, landscape or material, takes its
// parts from `parts`, none when the table lacks its path.
static void parts_give(voe_3d_models *models, const voe_game_parts *parts)
{
	VOE_BASE_ASSERT(models != NULL && parts != NULL, "no store or parts");
	for (uint32_t i = 0; i < voe_3d_models_count(models); i++) {
		const voe_3d_model_entry *entry = voe_3d_models_at(models, i);

		if (entry->loaded && !entry->picture && !entry->material &&
		    entry->landscape == NULL)
			voe_3d_models_parts_set(models, entry->path,
						find_parts(parts, entry->path));
	}
}

// `path`'s index in `table`, its count when it has none.
static uint32_t find(const voe_game_materials *table, const char *path)
{
	VOE_BASE_ASSERT(table != NULL && path != NULL, "no table or path");
	for (uint32_t i = 0; i < table->count; i++)
		if (strcmp(table->materials[i].path, path) == 0)
			return i;
	return table->count;
}

// The table's materials one call loads, by index, each once.
struct gathered {
	uint32_t *indices;
	uint32_t count;
};

// Gathers `path` from `table` when it is not empty, the store lacks it and it
// is not gathered yet, or keeps it failed and counts it when the table has
// none. False when the store has no room for it: the call gathers no more.
static bool gather(voe_3d_models *models, const voe_game_materials *table,
		   const char *path, struct gathered *gathered,
		   voe_game_models_failures *failures)
{
	uint32_t index;

	if (path[0] == '\0' || voe_3d_models_find(models, path) != NULL)
		return true;
	if (voe_3d_models_count(models) + gathered->count >= VOE_3D_MODELS)
		return false;
	index = find(table, path);
	if (index == table->count) {
		voe_base_report_subject_push(path);
		VOE_BASE_ERROR("game", "the project has no such material");
		voe_base_report_subject_pop();
		voe_3d_models_fail(models, path, 0);
		voe_game_models_failure_count(failures, models, path);
		return true;
	}
	for (uint32_t i = 0; i < gathered->count; i++)
		if (gathered->indices[i] == index)
			return true;
	gathered->indices[gathered->count++] = index;
	return true;
}

// Every path the world's shapes, then the store's entries, name.
static void gather_all(const voe_ecs_world *world, voe_3d_models *models,
		       const voe_game_materials *table,
		       struct gathered *gathered,
		       voe_game_models_failures *failures)
{
	uint32_t shapes = voe_3d_shape_count(world);
	bool room = true;

	for (uint32_t i = 0; room && i < shapes; i++)
		room = gather(models, table, voe_3d_shape_rows(world)[i].material,
			      gathered, failures);
	// An entry's index is stable as the store grows, and only a failed
	// material entry, naming none, is added.
	for (uint32_t i = 0; room && i < voe_3d_models_count(models); i++) {
		uint32_t named = entry_paths(voe_3d_models_at(models, i));

		for (uint32_t k = 0; room && k < named; k++)
			room = gather(models, table,
				      entry_path_at(voe_3d_models_at(models, i),
						    k),
				      gathered, failures);
	}
}

// The materials call, its map copies taken and written as `cooking` says, or
// its maps queued in `compressing`, reporting into `progress`.
static voe_game_models_failures
materials_load(const voe_ecs_world *world, voe_3d_models *models,
	       voe_render_device *device, const char *folder,
	       const voe_game_materials *table, const voe_game_parts *parts,
	       voe_base_arena *scratch, enum voe_game_models_cooking cooking,
	       voe_game_progress *progress, voe_game_compressing *compressing)
{
	struct voe_base_arena_mark mark;
	voe_game_models_failures failures = { 0 };
	voe_game_models_times times = { 0 };
	struct gathered gathered = { 0 };
	struct chunk chunk = { .folder = folder,
			       .count = 1,
			       .cooking = cooking,
			       .compressing = compressing,
			       .progress = progress };

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && table != NULL && parts != NULL &&
				scratch != NULL,
			"no folder, table, parts or scratch");
	mark = voe_base_arena_mark(scratch);
	parts_give(models, parts);
	gathered.indices = voe_base_arena_push(
		scratch, VOE_3D_MODELS * sizeof(*gathered.indices));
	gather_all(world, models, table, &gathered, &failures);
	chunk.materials = voe_base_arena_push(scratch, sizeof(*chunk.materials));
	chunk.total = gathered.count;
	for (uint32_t i = 0; i < gathered.count; i++) {
		voe_base_error error;

		if (voe_game_progress_stopped(progress))
			break;
		chunk.done = i;
		voe_game_progress_set(progress, "Loading materials", i,
				      gathered.count);
		*chunk.materials = (struct chunk_material){
			.material = &table->materials[gathered.indices[i]]
		};
		chunk_prepare(&chunk, &times);
		if (!material_put(chunk.materials, models, device, 0, &error,
				  &times))
			voe_game_models_failure_count(
				&failures, models, chunk.materials->material->path);
	}
	voe_game_models_times_write(&times, "materials",
				    VOE_GAME_MATERIALS_LINE);
	voe_base_arena_rewind(scratch, mark);
	return failures;
}

voe_game_models_failures
voe_game_models_materials(const voe_ecs_world *world, voe_3d_models *models,
			  voe_render_device *device, const char *folder,
			  const voe_game_materials *table,
			  const voe_game_parts *parts, voe_base_arena *scratch,
			  voe_game_progress *progress,
			  voe_game_compressing *compressing)
{
	return materials_load(world, models, device, folder, table, parts,
			      scratch, VOE_GAME_MODELS_COOK_STALE, progress,
			      compressing);
}

voe_game_models_failures voe_game_models_materials_cooked(
	const voe_ecs_world *world, voe_3d_models *models,
	voe_render_device *device, const char *folder,
	const voe_game_materials *table, const voe_game_parts *parts,
	voe_base_arena *scratch, voe_game_progress *progress)
{
	return materials_load(world, models, device, folder, table, parts,
			      scratch, VOE_GAME_MODELS_COOK_MISSING, progress,
			      NULL);
}
