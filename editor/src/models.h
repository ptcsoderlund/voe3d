// The editor's one model store (3d/models.h, ADR-0277): every model file the
// project's things name, loaded from the project folder, re-read when a file
// changes, emptied when a different project is in place, and handed to the
// pick read and to both view passes so models are drawn, picked and outlined.
// main.c makes it after the shapes are uploaded and destroys it before the
// device:
//
//     models = voe_editor_models_new();
//     voe_editor_models_update(models, &session, gpu, arena, now, NULL);
//     voe_editor_pick_read(..., voe_editor_models_store(models), ...);
//     voe_editor_models_destroy(models, gpu);
//
// ONE FOR THE PROGRAM, NOT ONE PER PROJECT. What the store holds lives on the
// device, and the device outlives every project the editor opens: New and Open
// swap the project and keep the device. So the store is made once beside the
// device and emptied when the session's folder differs from the one it last
// read against; a store per project would be torn down and made again at every
// swap for nothing. An untitled project has no folder to read from, so its
// model rows draw as nothing until it is saved.
//
// BETWEEN FRAMES, because a load waits for the card to go idle
// (game/models.h): the update runs after the world step, so this frame's rows
// are read, and before the frame's draw opens. During a splash wait a worker
// may call it instead (0370 point 4), with the wait's progress, which
// game/models.h reports into and stops on; every other call passes NULL.
//
// ONCE A SECOND the store's files are asked for their stamps (0277 point 5):
// often enough for a person exporting from Blender, and not a file system
// call per model every frame.
//
// A BROKEN FILE sets the session notice to `Could not read <path>: <reason>`
// (0277 point 9), the reason being the last phrase of base/report.h's first
// kept error: the lowest site that refused the file ("a file too short to hold
// even the header"), else game/models.c's category. The thing draws as
// nothing and the file is not read again until its stamp changes, so the
// notice is set once, not once a frame.
#pragma once

#include "session.h"

#include <3d/models.h>

#include <base/arena.h>

#include <game/progress.h>

#include <render/device.h>

typedef struct voe_editor_models voe_editor_models;

// An empty store. Never NULL: running out of memory is an assert.
voe_editor_models *voe_editor_models_new(void);

// Frees every entry's parts through `device`, then the store. NULL is nothing.
// Between frames only, before the device is destroyed.
void voe_editor_models_destroy(voe_editor_models *models,
			       voe_render_device *device);

// Once a frame, between frames: empties the store on a different folder, then
// reads the paths it lacks and, a second past the last look, re-reads changed
// files. `scratch` holds one file at a time and is rewound; `now` is the frame
// clock's, in seconds. `progress` is the splash wait's, NULL outside one.
void voe_editor_models_update(voe_editor_models *models,
			      voe_editor_session *session,
			      voe_render_device *device,
			      voe_base_arena *scratch, double now,
			      voe_game_progress *progress);

// The store to read, for what draws, picks and outlines.
const voe_3d_models *voe_editor_models_store(const voe_editor_models *models);
