// The loop's splash waits (waits.h): the prepare, the reload and the load,
// moved from main.c as they were.
#include "waits.h"

#include "loading.h"

#include <base/assert.h>

#include <game/starting.h>

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
