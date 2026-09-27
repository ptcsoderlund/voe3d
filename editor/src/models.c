// The editor's one model store models.h describes: made empty, emptied on a
// different folder, filled and re-read through game/models.h once a frame,
// cleared through the device and destroyed, and handed out read-only.
#include "models.h"

#include <base/assert.h>
#include <base/report.h>

#include <game/models.h>

#include <stdlib.h>
#include <string.h>

// How often the store's files are asked for their stamps, in seconds.
#define WATCH_SECONDS 1.0

struct voe_editor_models {
	voe_3d_models *store;
	// The folder the store was last read against, the store's own copy;
	// NULL before any and while the project is untitled.
	char *folder;
	// The frame clock's reading at the last watch.
	double looked;
};

voe_editor_models *voe_editor_models_new(void)
{
	voe_editor_models *models = calloc(1, sizeof(*models));

	VOE_BASE_ASSERT(models != NULL, "out of memory making the model store");
	models->store = voe_3d_models_new();
	VOE_BASE_ASSERT(models->store != NULL, "made a model store with none");
	return models;
}

void voe_editor_models_destroy(voe_editor_models *models,
			       voe_render_device *device)
{
	if (models == NULL)
		return;
	VOE_BASE_ASSERT(device != NULL, "clearing models with no device");
	voe_3d_models_clear(models->store, device);
	voe_3d_models_destroy(models->store);
	free(models->folder);
	free(models);
}

// Empties the store and keeps a copy of `folder` (NULL for untitled) when it
// is not the one last read against.
static void follow_folder(voe_editor_models *models, const char *folder,
			  voe_render_device *device)
{
	if (folder == NULL && models->folder == NULL)
		return;
	if (folder != NULL && models->folder != NULL &&
	    strcmp(folder, models->folder) == 0)
		return;
	voe_3d_models_clear(models->store, device);
	free(models->folder);
	models->folder = NULL;
	if (folder != NULL) {
		models->folder = strdup(folder);
		VOE_BASE_ASSERT(models->folder != NULL,
				"out of memory copying the models' folder");
	}
	VOE_BASE_ASSERT(voe_3d_models_count(models->store) == 0,
			"a cleared store still holds models");
}

// The notice for a call that failed: the first path and the reason, the last
// phrase of the first error kept since the caller's clear.
static void say_failure(voe_editor_notice *notice,
			voe_game_models_failures failures)
{
	const char *kept = voe_base_report_error_first();
	const char *category = kept != NULL ? strrchr(kept, ':') : NULL;

	VOE_BASE_ASSERT(notice != NULL, "saying a failure into no notice");
	if (failures.count == 0)
		return;
	if (category != NULL)
		category += category[1] == ' ' ? 2 : 1;
	else
		category = kept != NULL ? kept : "could not be read";
	voe_editor_notice_set(notice, "Could not read %s: %s",
			      failures.first != NULL ? failures.first : "a model",
			      category);
}

void voe_editor_models_update(voe_editor_models *models,
			      voe_editor_session *session,
			      voe_render_device *device,
			      voe_base_arena *scratch, double now)
{
	VOE_BASE_ASSERT(models != NULL && session != NULL,
			"updating no store or no session");
	VOE_BASE_ASSERT(session->project != NULL && device != NULL &&
				scratch != NULL,
			"updating models with no project, device or scratch");
	follow_folder(models, session->project->folder, device);
	if (models->folder == NULL)
		return;
	voe_base_report_error_clear();
	say_failure(&session->notice,
		    voe_game_models_update(session->project->world,
					   models->store, device,
					   models->folder, scratch));
	if (now - models->looked < WATCH_SECONDS)
		return;
	models->looked = now;
	voe_base_report_error_clear();
	say_failure(&session->notice,
		    voe_game_models_watch(models->store, device, models->folder,
					  scratch));
}

const voe_3d_models *voe_editor_models_store(const voe_editor_models *models)
{
	VOE_BASE_ASSERT(models != NULL, "reading no model store");
	VOE_BASE_ASSERT(models->store != NULL, "a model store with none");
	return models->store;
}
