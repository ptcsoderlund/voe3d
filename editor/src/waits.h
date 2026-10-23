// The editor loop's splash waits between frames, lifted out of main.c's loop:
// what the session asked for last frame, carried out before this frame's
// session step. The works themselves are loading.h's; this file only fills
// their contexts, waits on them behind the splash and acts on the outcome.
//
//     voe_editor_waits waits = { .app = app, .ui = ui, .scratch = scratch,
//             .splash = splash_held ? &splash : NULL, .session = &session,
//             .scene = &scene, .models = models, .device = gpu,
//             .worker_scratch = worker_scratch };
//     ...
//     if (!voe_editor_waits_run(&waits))
//             ... a false wait: the program ends as a closed window does
//
// IN ORDER: an import's or Make levels' prepare (`prepare_due`), its model
// loaded again unless the prepare refused it, whose line becomes the notice
// (0431, 0445 point 4); a swaps-only Make's reload (`reload_due`, 0436 point
// 8); then New or Open's load (`load_due`). The prepare comes before any load
// replaces the project it was imported into.
//
// Constraints: the parts pointed at are main.c's and outlive the loop. During
// a wait the worker owns the session, scene and models (loading.h); `scratch`
// is the main thread's frame arena and is left empty after a wait.
#pragma once

#include "models.h"
#include "scene.h"
#include "session.h"

#include <app/app.h>
#include <app/picture.h>

#include <base/arena.h>

#include <render/device.h>

#include <ui/widgets.h>

#include <stdbool.h>

typedef struct {
	voe_app *app;
	voe_ui_context *ui;
	// The main thread's frame scratch the splash draws with.
	voe_base_arena *scratch;
	// The engine's splash, or NULL for the plain screen (splash.h).
	const voe_app_picture *splash;
	voe_editor_session *session;
	voe_editor_scene *scene;
	voe_editor_models *models;
	voe_render_device *device;
	// The worker's own arena (loading.h).
	voe_base_arena *worker_scratch;
} voe_editor_waits;

// The due prepare, reload and load, each cleared or carried out. False where a
// wait answered false: a closing window, or a stop, already on stderr.
[[nodiscard]] bool voe_editor_waits_run(voe_editor_waits *waits);
