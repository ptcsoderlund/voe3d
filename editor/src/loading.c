// The two works loading.h names, each run once on a splash wait's worker in
// the worker's own scratch, which is rewound before it returns.
#include "loading.h"

#include <app/pipeline_cache.h>

#include <base/assert.h>
#include <base/error.h>

#include <game/starting.h>

bool voe_editor_loading_start_work(void *context, voe_game_progress *progress)
{
	voe_editor_loading_start *start = context;
	voe_base_error error = VOE_BASE_OK;
	bool prepared;

	VOE_BASE_ASSERT(start != NULL && start->device != NULL &&
				start->scratch != NULL && start->log != NULL,
			"a start's work with no device, scratch or log");
	VOE_BASE_ASSERT(start->session != NULL && start->models != NULL &&
				start->shapes != NULL,
			"a start's work with no session, models or shapes");
	prepared = voe_game_starting_shaders(
		start->device,
		voe_app_pipeline_cache_path(start->scratch,
					    "pipelines_editor.cache"),
		start->scratch, progress);
	voe_base_arena_clear(start->scratch);
	if (!prepared)
		return false;
	voe_app_start_log_step(start->log, "preparing shaders");

	voe_game_progress_set(progress, "Uploading shapes", 0, 0);
	if (voe_game_progress_stopped(progress))
		return false;
	// render says why on stderr when it refuses.
	if (!voe_3d_shapes_upload(start->device, start->shapes, &error)) {
		start->failed = true;
		return false;
	}
	voe_editor_models_update(start->models, start->session, start->device,
				 start->scratch, 0.0, progress);
	voe_base_arena_clear(start->scratch);
	return !voe_game_progress_stopped(progress);
}

bool voe_editor_loading_load_work(void *context, voe_game_progress *progress)
{
	voe_editor_loading_load *load = context;

	VOE_BASE_ASSERT(load != NULL && load->device != NULL &&
				load->scratch != NULL,
			"a load's work with no device or scratch");
	VOE_BASE_ASSERT(load->session != NULL && load->scene != NULL &&
				load->models != NULL,
			"a load's work with no session, scene or models");
	voe_game_progress_set(progress, "Loading scene", 0, 0);
	if (voe_game_progress_stopped(progress))
		return false;
	voe_editor_session_load(load->session, load->scene);
	voe_editor_models_update(load->models, load->session, load->device,
				 load->scratch, 0.0, progress);
	voe_base_arena_clear(load->scratch);
	return !voe_game_progress_stopped(progress);
}
