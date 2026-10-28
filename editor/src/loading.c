// The four works loading.h names, each run once on a splash wait's worker in
// the worker's own scratch, which is rewound before it returns.
#include "loading.h"

#include <app/pipeline_cache.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <game/models.h>
#include <game/starting.h>

#include <platform/path.h>

#include <stdio.h>

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

bool voe_editor_loading_prepare_work(void *context, voe_game_progress *progress)
{
	voe_editor_loading_prepare *prepare = context;
	voe_base_error error = VOE_BASE_OK;
	bool made = false;

	VOE_BASE_ASSERT(prepare != NULL && prepare->device != NULL &&
				prepare->scratch != NULL,
			"a prepare's work with no device or scratch");
	VOE_BASE_ASSERT(prepare->folder != NULL && prepare->path != NULL,
			"a prepare's work with no folder or path");
	snprintf(prepare->phase, sizeof prepare->phase, "Preparing %s",
		 voe_platform_path_name(prepare->path));
	voe_game_progress_set(progress, prepare->phase, 0, 0);
	if (voe_game_progress_stopped(progress))
		return false;
	// The kept error is this thread's, so the refusal is built here.
	voe_base_report_error_clear();
	if (!voe_game_models_prepare(prepare->folder, prepare->path,
				     prepare->device, prepare->scratch,
				     prepare->remake, &made, &error)) {
		prepare->refused = true;
		voe_editor_notice_from_report(&prepare->refusal, prepare->path);
	}
	voe_base_arena_clear(prepare->scratch);
	return !voe_game_progress_stopped(progress);
}

bool voe_editor_loading_cook_work(void *context, voe_game_progress *progress)
{
	voe_editor_loading_cook *cook = context;
	bool cooked;

	VOE_BASE_ASSERT(cook != NULL && cook->device != NULL &&
				cook->scratch != NULL && cook->folder != NULL,
			"a cook's work with no device, scratch or folder");
	VOE_BASE_ASSERT(cook->paths != NULL || cook->path_count == 0,
			"a cook's work counting paths it lacks");
	cooked = voe_game_models_cook(cook->folder, cook->paths,
				      cook->path_count, &cook->table,
				      cook->device, cook->scratch, progress);
	voe_base_arena_clear(cook->scratch);
	return cooked;
}
