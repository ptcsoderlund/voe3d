// The model loader of game/include/game/models.h: a model row's path or an
// emitter's texture joined onto the folder, stamped, read into scratch and
// handed to the store, the soft dot loaded when any emitter is there and the
// water record when any water is, each
// failure one line on stderr and counted. Scratch is rewound to where it stood after each
// file, so a caller's own scratch data survives the call.
#include <game/models.h>

#include <3d/emitter_component.h>
#include <3d/model_component.h>
#include <3d/water_component.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stddef.h>

// Counts one failure of `path`, the store's copy kept when it is the first.
static void count_failure(voe_game_models_failures *failures,
			  const voe_3d_models *models, const char *path,
			  voe_base_error error)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, path);

	VOE_BASE_ASSERT(failures != NULL, "counting into no failures");
	VOE_BASE_ASSERT(path != NULL, "counting a failure with no path");
	VOE_BASE_ERROR("game", "could not read the model %s: %s", path,
		       voe_base_error_string(error));
	// The dot keeps no failed entry; its "" is a literal, valid for ever.
	if (failures->count == 0 && entry != NULL)
		failures->first = entry->path;
	else if (failures->count == 0 && path[0] == '\0')
		failures->first = "";
	failures->count++;
}

// Loads `path` from `folder` if its stamp is not `known`; a file with no
// stamp becomes a failed entry at 0, unless it already is one. False, with
// the category in `error`, when it failed.
static bool load_changed(voe_3d_models *models, voe_render_device *device,
			 const char *folder, const char *path, uint64_t known,
			 voe_base_arena *scratch, voe_base_error *error)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const char *full = voe_platform_path_join(scratch, folder, path);
	const uint8_t *bytes = NULL;
	uint64_t stamp = 0;
	size_t size = 0;
	bool loaded = true;

	VOE_BASE_ASSERT(path[0] != '\0', "loading a model with no path");
	*error = VOE_BASE_OK;
	if (!voe_platform_file_stamp(full, &stamp)) {
		if (known != 0 || voe_3d_models_find(models, path) == NULL) {
			voe_3d_models_fail(models, path, 0);
			*error = VOE_BASE_ERROR_UNAVAILABLE;
			loaded = false;
		}
	} else if (stamp != known) {
		bytes = voe_platform_file_read(full, scratch, &size, error);
		if (bytes == NULL)
			voe_3d_models_fail(models, path, 0);
		loaded = bytes != NULL &&
			 voe_3d_models_load(models, device, path, stamp, bytes,
					    size, error);
	}
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(loaded || *error != VOE_BASE_OK,
			"a failed model load with no category");
	return loaded;
}

// Loads `path` when it is not empty and the store lacks it, counting a
// failure. False when the store is full, which ends the update.
static bool load_new(voe_3d_models *models, voe_render_device *device,
		     const char *folder, const char *path,
		     voe_base_arena *scratch,
		     voe_game_models_failures *failures)
{
	voe_base_error error;

	VOE_BASE_ASSERT(path != NULL, "loading no path");
	VOE_BASE_ASSERT(failures != NULL, "counting into no failures");
	if (path[0] == '\0' || voe_3d_models_find(models, path) != NULL)
		return true;
	if (voe_3d_models_count(models) == VOE_3D_MODELS)
		return false;
	if (!load_changed(models, device, folder, path, 0, scratch, &error))
		count_failure(failures, models, path, error);
	return true;
}

voe_game_models_failures voe_game_models_update(const voe_ecs_world *world,
						voe_3d_models *models,
						voe_render_device *device,
						const char *folder,
						voe_base_arena *scratch)
{
	const voe_3d_model *rows = voe_3d_model_rows(world);
	const voe_3d_emitter *emitters = voe_3d_emitter_rows(world);
	voe_game_models_failures failures = { 0 };
	bool room = true;
	voe_base_error error;

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && scratch != NULL,
			"no folder or scratch");
	for (uint32_t row = 0; room && row < voe_3d_model_count(world); row++)
		room = load_new(models, device, folder, rows[row].path, scratch,
				&failures);
	for (uint32_t row = 0; room && row < voe_3d_emitter_count(world); row++)
		room = load_new(models, device, folder, emitters[row].texture,
				scratch, &failures);
	if (voe_3d_emitter_count(world) > 0 &&
	    voe_3d_models_find(models, "") == NULL &&
	    !voe_3d_models_load_dot(models, device, &error))
		count_failure(&failures, models, "", error);
	if (voe_3d_water_count(world) > 0 &&
	    voe_3d_models_water(models) == NULL &&
	    !voe_3d_models_load_water(models, device, &error))
		count_failure(&failures, models, "", error);
	return failures;
}

voe_game_models_failures voe_game_models_watch(voe_3d_models *models,
					       voe_render_device *device,
					       const char *folder,
					       voe_base_arena *scratch)
{
	voe_game_models_failures failures = { 0 };

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && scratch != NULL,
			"no folder or scratch");
	for (uint32_t i = 0; i < voe_3d_models_count(models); i++) {
		const voe_3d_model_entry *entry = voe_3d_models_at(models, i);
		voe_base_error error;

		if (!load_changed(models, device, folder, entry->path,
				  entry->stamp, scratch, &error))
			count_failure(&failures, models,
				      voe_3d_models_at(models, i)->path, error);
	}
	return failures;
}
