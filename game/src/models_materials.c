// The material loader of game/include/game/models.h: every loaded model entry
// given its parts from the parts table; a table entry's four maps joined onto
// the folder, read into scratch and handed to the store with the values; every
// material path a shape names, then every model part's `wears` and loaded
// landscape's layer, that the store lacks looked up in the table, loaded or
// kept failed, each failure one line on stderr and counted. Scratch is rewound
// after each material.
//
// Constraints: a path is found in a table by comparing it with every entry: a
// scan per unread material path, once each since a failure is kept, and a scan
// of the parts table per model entry per call, each frame in the game. Sorted
// tables would lift it.
#include <game/models.h>

#include <3d/shape_component.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stddef.h>
#include <string.h>

// Reads `path` from `folder` into `map`; "" leaves it empty. False, with the
// category in `error`, when the file will not read.
static bool read_map(const char *folder, const char *path,
		     voe_base_arena *scratch, voe_3d_material_map *map,
		     voe_base_error *error)
{
	VOE_BASE_ASSERT(path != NULL && map != NULL, "reading no map");
	*map = (voe_3d_material_map){ 0 };
	if (path[0] == '\0')
		return true;
	map->bytes = voe_platform_file_read(
		voe_platform_path_join(scratch, folder, path), scratch,
		&map->size, error);
	VOE_BASE_ASSERT(map->bytes != NULL || *error != VOE_BASE_OK,
			"an unread map with no category");
	return map->bytes != NULL;
}

bool voe_game_models_material_load(voe_3d_models *models,
				   voe_render_device *device,
				   const char *folder,
				   const voe_game_material *material,
				   uint64_t stamp, voe_base_arena *scratch,
				   voe_base_error *error)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const voe_assets_material_file *values = &material->values;
	voe_3d_material_maps maps;
	bool loaded;

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(material != NULL && material->path[0] != '\0',
			"loading a material with no path");
	*error = VOE_BASE_OK;
	loaded = read_map(folder, values->colormap, scratch, &maps.colour,
			  error) &&
		 read_map(folder, values->normalmap, scratch, &maps.normal,
			  error) &&
		 read_map(folder, values->ormmap, scratch, &maps.orm, error) &&
		 read_map(folder, values->emissionmap, scratch, &maps.emission,
			  error);
	if (!loaded)
		voe_3d_models_fail(models, material->path, stamp);
	else
		loaded = voe_3d_models_load_material(models, device,
						     material->path, stamp,
						     values, maps, error);
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

// `path`'s entry in `table`, NULL when it has none.
static const voe_game_material *find(const voe_game_materials *table,
				     const char *path)
{
	VOE_BASE_ASSERT(table != NULL && path != NULL, "no table or path");
	for (uint32_t i = 0; i < table->count; i++)
		if (strcmp(table->materials[i].path, path) == 0)
			return &table->materials[i];
	return NULL;
}

// Counts one failure of `path`, its kept entry's copy named when the first.
static void count_failure(voe_game_models_failures *failures,
			  const voe_3d_models *models, const char *path,
			  voe_base_error error)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, path);

	VOE_BASE_ASSERT(failures != NULL && path != NULL, "no failures or path");
	VOE_BASE_ERROR("game", "could not load the material %s: %s", path,
		       voe_base_error_string(error));
	if (failures->count == 0 && entry != NULL)
		failures->first = entry->path;
	failures->count++;
}

// Loads `path` from `table` when it is not empty and the store lacks it, or
// keeps it failed and counts it when the table has none. False when the
// store is full: it said so once, and the call ends.
static bool load_missing(voe_3d_models *models, voe_render_device *device,
			 const char *folder, const voe_game_materials *table,
			 const char *path, voe_base_arena *scratch,
			 voe_game_models_failures *failures)
{
	const voe_game_material *material;
	voe_base_error error = VOE_BASE_ERROR_UNAVAILABLE;

	if (path[0] == '\0' || voe_3d_models_find(models, path) != NULL)
		return true;
	if (voe_3d_models_count(models) == VOE_3D_MODELS)
		return false;
	material = find(table, path);
	if (material == NULL)
		voe_3d_models_fail(models, path, 0);
	else if (voe_game_models_material_load(models, device, folder,
					       material, 0, scratch, &error))
		return true;
	count_failure(failures, models, path, error);
	return true;
}

voe_game_models_failures
voe_game_models_materials(const voe_ecs_world *world, voe_3d_models *models,
			  voe_render_device *device, const char *folder,
			  const voe_game_materials *table,
			  const voe_game_parts *parts, voe_base_arena *scratch)
{
	uint32_t shapes = voe_3d_shape_count(world);
	voe_game_models_failures failures = { 0 };
	bool room = true;

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && table != NULL && parts != NULL &&
				scratch != NULL,
			"no folder, table, parts or scratch");
	parts_give(models, parts);
	for (uint32_t i = 0; room && i < shapes; i++)
		room = load_missing(models, device, folder, table,
				    voe_3d_shape_rows(world)[i].material,
				    scratch, &failures);
	// The parts' and landscapes' paths; an entry's index is stable as the
	// store grows, and only a new material entry, naming none, is added.
	for (uint32_t i = 0; room && i < voe_3d_models_count(models); i++) {
		uint32_t named = entry_paths(voe_3d_models_at(models, i));

		for (uint32_t k = 0; room && k < named; k++)
			room = load_missing(
				models, device, folder, table,
				entry_path_at(voe_3d_models_at(models, i), k),
				scratch, &failures);
	}
	return failures;
}
