// The material loader of game/include/game/models.h: a table entry's maps
// joined onto the folder, read into scratch and handed to the store with the
// values; every material path a shape or model row names that the store lacks
// looked up in the table, loaded or kept failed, each failure one line on
// stderr and counted. Scratch is rewound after each material.
//
// Constraints: a path is found in the table by comparing it with every entry,
// a scan per unread path, once each since a failure is kept; a sorted table
// would lift it.
#include <game/models.h>

#include <3d/model_component.h>
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
	loaded = read_map(folder, values->colour_map, scratch, &maps.colour,
			  error) &&
		 read_map(folder, values->normal_map, scratch, &maps.normal,
			  error) &&
		 read_map(folder, values->roughness_map, scratch,
			  &maps.roughness, error);
	if (!loaded)
		voe_3d_models_fail(models, material->path, stamp);
	else
		loaded = voe_3d_models_load_material(models, device,
						     material->path, stamp,
						     values, maps, error);
	voe_base_arena_rewind(scratch, mark);
	return loaded;
}

// The world's `index`th material path: the shapes', then each model row's
// VOE_3D_MODEL_MATERIALS in turn.
static const char *path_at(const voe_ecs_world *world, uint32_t index)
{
	uint32_t shapes = voe_3d_shape_count(world);

	if (index < shapes)
		return voe_3d_shape_rows(world)[index].material;
	index -= shapes;
	return voe_3d_model_rows(world)[index / VOE_3D_MODEL_MATERIALS]
		.materials[index % VOE_3D_MODEL_MATERIALS];
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

voe_game_models_failures
voe_game_models_materials(const voe_ecs_world *world, voe_3d_models *models,
			  voe_render_device *device, const char *folder,
			  const voe_game_materials *table,
			  voe_base_arena *scratch)
{
	uint32_t paths = voe_3d_shape_count(world) +
			 voe_3d_model_count(world) * VOE_3D_MODEL_MATERIALS;
	voe_game_models_failures failures = { 0 };

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && table != NULL && scratch != NULL,
			"no folder, table or scratch");
	for (uint32_t i = 0; i < paths; i++) {
		const char *path = path_at(world, i);
		const voe_game_material *material;
		voe_base_error error = VOE_BASE_ERROR_UNAVAILABLE;

		if (path[0] == '\0' || voe_3d_models_find(models, path) != NULL)
			continue;
		// A full store said so once; the call ends.
		if (voe_3d_models_count(models) == VOE_3D_MODELS)
			break;
		material = find(table, path);
		if (material == NULL)
			voe_3d_models_fail(models, path, 0);
		else if (voe_game_models_material_load(models, device, folder,
						       material, 0, scratch,
						       &error))
			continue;
		count_failure(&failures, models, path, error);
	}
	return failures;
}
