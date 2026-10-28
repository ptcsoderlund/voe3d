// The loop's splash waits (waits.h): the prepare, the reload and the load,
// moved from main.c as they were, and the cook before Play and Ship.
#include "waits.h"

#include "game_tree_find.h"
#include "loading.h"
#include "materials.h"

#include <base/assert.h>

#include <game/models.h>
#include <game/starting.h>

#include <platform/path.h>

#include <stdio.h>
#include <string.h>

// Every `.glb` under `<folder>/Assets/` as `Assets/...`, in the main thread's
// scratch, which the splash only draws above. NULL with the notice set when
// the find failed; `count` is then 0.
static const char *const *waits_models_find(voe_editor_waits *waits,
					    const char *folder,
					    uint32_t *count)
{
	struct voe_editor_game_tree_found found = { 0 };
	const char **paths;

	VOE_BASE_ASSERT(folder != NULL && count != NULL,
			"finding models in no folder or with no count");
	*count = 0;
	if (!voe_editor_game_tree_find(
		    voe_platform_path_join(waits->scratch, folder, "Assets"),
		    ".glb", waits->scratch, &found, &waits->session->notice))
		return NULL;
	paths = voe_base_arena_push(waits->scratch,
				    (found.count + 1) * sizeof *paths);
	for (uint32_t i = 0; i < found.count; i++) {
		size_t size = strlen(found.items[i].path) + sizeof "Assets/";
		char *path = voe_base_arena_push(waits->scratch, size);

		snprintf(path, size, "Assets/%s", found.items[i].path);
		paths[i] = path;
	}
	*count = found.count;
	VOE_BASE_ASSERT(paths != NULL, "found models with no list");
	return paths;
}

// The cook before Play and Ship (waits.h): the `.glb`s found and the stale
// asked on this thread, then a wait only when any is. A failed find builds
// anyway. False where the wait was false.
static bool waits_cook(voe_editor_waits *waits)
{
	voe_editor_session *session = waits->session;
	voe_editor_loading_cook cook = {
		.folder = session->project->folder,
		.table = voe_editor_materials_table(
			voe_editor_models_materials(waits->models)),
		.device = waits->device,
		.scratch = waits->worker_scratch };

	VOE_BASE_ASSERT(session->cook_due && cook.folder != NULL,
			"cooking with none due or no folder");
	cook.paths = waits_models_find(waits, cook.folder, &cook.path_count);
	if (cook.paths != NULL &&
	    voe_game_models_stale(cook.folder, cook.paths, cook.path_count,
				  &cook.table, waits->scratch) > 0 &&
	    !voe_game_starting_wait(waits->app, waits->ui, waits->scratch,
				    waits->splash, voe_editor_loading_cook_work,
				    &cook))
		return false;
	voe_base_arena_clear(waits->scratch);
	voe_editor_session_cooked(session);
	VOE_BASE_ASSERT(!session->cook_due, "a cooked build still due");
	return true;
}

bool voe_editor_waits_run(voe_editor_waits *waits)
{
	voe_editor_session *session;

	VOE_BASE_ASSERT(waits != NULL && waits->session != NULL,
			"running the loop's waits with no waits or session");
	session = waits->session;

	// AN IMPORTED MODEL'S LEVELS MADE BEHIND THE SPLASH (loading.h), before
	// any load replaces the project it was imported into. The model is
	// loaded again so its new levels draw at once (0431); a refused prepare
	// puts its line in the notice and is not reloaded, which would only
	// meet the same cause again (0445 point 4).
	if (session->prepare_due) {
		voe_editor_loading_prepare prepare = {
			.folder = session->project->folder,
			.path = session->prepare_path,
			.device = waits->device,
			.scratch = waits->worker_scratch,
			.remake = session->prepare_remake };
		if (!voe_game_starting_wait(waits->app, waits->ui,
					    waits->scratch, waits->splash,
					    voe_editor_loading_prepare_work,
					    &prepare))
			return false;
		voe_base_arena_clear(waits->scratch);
		if (prepare.refused)
			session->notice = prepare.refusal;
		else
			voe_editor_models_reload(waits->models, session,
						 waits->device,
						 session->prepare_path,
						 waits->scratch);
		session->prepare_due = false;
		session->prepare_remake = false;
	}

	// A MAKE THAT CHANGED ONLY SWAPS needs no splash: the levels file is
	// fresh, and the game's loader patches the new swaps into it on this
	// reload, twenty bytes and no levels made (0436 point 8).
	if (session->reload_due) {
		voe_editor_models_reload(waits->models, session, waits->device,
					 session->prepare_path, waits->scratch);
		session->reload_due = false;
	}

	// PLAY OR SHIP'S COOK, before any load replaces the project it is for.
	if (session->cook_due && !waits_cook(waits))
		return false;

	// NEW OR OPEN'S LOAD ON A WORKER BEHIND THE SPLASH (loading.h).
	if (session->load_due) {
		voe_editor_loading_load load = {
			.device = waits->device,
			.scratch = waits->worker_scratch,
			.session = session, .scene = waits->scene,
			.models = waits->models };
		if (!voe_game_starting_wait(waits->app, waits->ui,
					    waits->scratch, waits->splash,
					    voe_editor_loading_load_work,
					    &load))
			return false;
		voe_base_arena_clear(waits->scratch);
	}
	return true;
}
