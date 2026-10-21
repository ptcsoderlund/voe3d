// The editor's three works for a splash wait (game/starting.h, 0370 point 4):
// the start, a scene load and an import's prepare, each a context the caller
// fills and a work function it hands to voe_game_starting_wait.
//
//     voe_editor_loading_start start = { .device = gpu, .scratch = worker,
//             .log = &log, .session = &session, .models = models,
//             .shapes = &shapes };
//     if (!voe_game_starting_wait(app, ui, scratch, splash,
//                                 voe_editor_loading_start_work, &start))
//             ... start.failed ? a refused upload : a closed window
//
// THE START is the shaders step with `<settings>/voe3d/pipelines_editor.cache`
// (0370 point 6), "preparing shaders" logged, then the built-in shapes uploaded
// ("Uploading shapes") and the project's models read with the wait's progress.
// A LOAD is voe_editor_session_load ("Loading scene") and the models read the
// same way. The project is opened before the window, so the start has none.
// A PREPARE ("Preparing <file>") makes an imported `.glb`'s levels file
// through voe_game_models_prepare (0426 point 3), no device in it, its levels
// made over the store's pool (0448 point 2); the Assets
// menu's Make levels runs the same one, made even when fresh (0429); main.c
// loads the model again after the wait (0431) unless the prepare refused it
// (0445 point 4).
//
// WHAT THE WORKER OWNS. During the wait the worker owns everything its context
// names: the session and its project, the scene, the model store, the shapes
// and the start log. The main thread touches none of them until the wait
// returns; it only polls the window and draws the splash. Uploads on the
// worker are safe because render guards its device against an open frame
// (0370 point 5).
//
// Constraints: `scratch` is the worker's own arena, never the main thread's,
// and is left empty. The models are read with a frame clock of 0, so the
// once-a-second re-read waits for the loop's own call (models.h).
#pragma once

#include "models.h"
#include "notice.h"
#include "scene.h"
#include "session.h"

#include <3d/model_component.h>
#include <3d/shape_system.h>

#include <app/start_log.h>

#include <base/arena.h>
#include <base/workers.h>

#include <game/progress.h>

#include <render/device.h>

#include <stdbool.h>

// The start's work. `failed` is set when the shapes would not upload, which
// render has said on stderr; a false without it is a stop or a refused prepare.
typedef struct {
	voe_render_device *device;
	voe_base_arena *scratch;
	voe_app_start_log *log;
	voe_editor_session *session;
	voe_editor_models *models;
	voe_3d_shapes *shapes;
	bool failed;
} voe_editor_loading_start;

// A New or Open load's work, the one `session->load_due` asks for.
typedef struct {
	voe_render_device *device;
	voe_base_arena *scratch;
	voe_editor_session *session;
	voe_editor_scene *scene;
	voe_editor_models *models;
} voe_editor_loading_load;

// "Preparing " and a model path's file name, its terminator included.
#define VOE_EDITOR_LOADING_PHASE (VOE_3D_MODEL_PATH + 16)

// An import's prepare, the one `session->prepare_due` asks for: `path` under
// `folder`, as game/models.h takes them. `phase` is the work's own, written
// by it and pointed at by the progress while the wait runs. `remake` is
// voe_game_models_prepare's: Import leaves it false, Make levels sets it.
// `workers` is the pool the levels are made over, the store's; NULL in order.
// `refused` is set when the prepare refused the `.glb`, whose cause has its
// own line on stderr; `refusal` is then that kept line as the notice.
typedef struct {
	const char *folder;
	const char *path;
	voe_base_arena *scratch;
	voe_base_workers *workers;
	bool remake;
	bool refused;
	voe_editor_notice refusal;
	char phase[VOE_EDITOR_LOADING_PHASE];
} voe_editor_loading_prepare;

// A voe_game_starting_work on a voe_editor_loading_start. False once stopped,
// on a prepare that failed, or with `failed` set on a refused upload.
[[nodiscard]] bool voe_editor_loading_start_work(void *context,
						 voe_game_progress *progress);

// A voe_game_starting_work on a voe_editor_loading_load. A failed open is the
// session's notice, not a false; false only once stopped.
[[nodiscard]] bool voe_editor_loading_load_work(void *context,
						voe_game_progress *progress);

// A voe_game_starting_work on a voe_editor_loading_prepare. A file that will
// not prepare sets `refused` and `refusal`, not a line of its own and not a
// false: its cause said its line where it was found. False only once stopped.
[[nodiscard]] bool voe_editor_loading_prepare_work(void *context,
						   voe_game_progress *progress);
