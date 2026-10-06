// The editor's two works for a splash wait (game/starting.h, 0370 point 4):
// the start and a scene load, each a context of pointers the caller fills and a
// work function it hands to voe_game_starting_wait.
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
#include "scene.h"
#include "session.h"

#include <3d/shape_system.h>

#include <app/start_log.h>

#include <base/arena.h>

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

// A voe_game_starting_work on a voe_editor_loading_start. False once stopped,
// on a prepare that failed, or with `failed` set on a refused upload.
[[nodiscard]] bool voe_editor_loading_start_work(void *context,
						 voe_game_progress *progress);

// A voe_game_starting_work on a voe_editor_loading_load. A failed open is the
// session's notice, not a false; false only once stopped.
[[nodiscard]] bool voe_editor_loading_load_work(void *context,
						voe_game_progress *progress);
