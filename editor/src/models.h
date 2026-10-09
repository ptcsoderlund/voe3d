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
//
// A LANDSCAPE IS EDITED IN THE STORE AND ONLY THROUGH THIS FILE (0379 points
// 4 and 6): its grid is the store's, so the calls into 3d/models.h that write
// and re-read it are made here and nowhere else. Each frame _frame, inside the
// draw, writes its dirty heights into its texture (0396 point 5). Save writes
// every edited one to its file; a New or Open reads every edited one again.
// The Landscape panel's size is written at once and re-read at the next
// update; one re-read waits at a time, enough for a panel that writes at most
// one a frame, and a second before the update would replace the first.
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

// Once a frame, after the draw opens and before any pass: the landscapes'
// dirty heights written into their textures, up to
// VOE_3D_LANDSCAPE_WRITE_TEXELS, the rest carried to the next frame.
void voe_editor_models_frame(voe_editor_models *models,
			     voe_render_device *device);

// Every edited landscape written to `<folder>/<path>` and marked saved. False
// at the first refused write, `why` naming the file; those before it saved.
// `scratch` is rewound.
[[nodiscard]] bool voe_editor_models_save(voe_editor_models *models,
					  const char *folder,
					  voe_base_arena *scratch,
					  voe_editor_notice *why);

// Every edited landscape read from `<folder>/<path>` again and loaded over
// itself. A file that will not read keeps its entry as an update's failure
// does, the reason on stderr. Nothing for a NULL folder. Between frames.
void voe_editor_models_revert(voe_editor_models *models, const char *folder,
			      voe_render_device *device,
			      voe_base_arena *scratch);

// One stamp of `brush` at (x, z) in `path`'s grid's own space over `seconds`
// (3d/models.h): the rect of heights it changed, empty for a path that is no
// loaded landscape. `scratch` is rewound.
voe_3d_landscape_rect voe_editor_models_brush(voe_editor_models *models,
					      const char *path,
					      const voe_3d_brush *brush,
					      float x, float z, float seconds,
					      voe_base_arena *scratch);

// `rect`'s heights, row-major, written into `path`'s landscape: an undo step's
// stroke put back (strokes.h). Nothing for a path that is no loaded landscape.
void voe_editor_models_put(voe_editor_models *models, const char *path,
			   voe_3d_landscape_rect rect, const float *values);

// Every entry at `from` or under `from/` takes `to` in its place, both under
// `Assets/`: a rename or move keeps a landscape's unsaved heights (0379 point 6).
void voe_editor_models_rename(voe_editor_models *models, const char *from,
			      const char *to);

// The size of `path`'s landscape (`Assets/...`) into `size`: the store's entry
// when loaded, else read from `<folder>/<path>` in `scratch`, which is
// rewound. False, `why` naming the file, when it will not read.
[[nodiscard]] bool voe_editor_models_landscape_size_found(
	voe_editor_models *models, const char *folder, const char *path,
	voe_base_arena *scratch, float *size, voe_editor_notice *why);

// `path`'s landscape written to `<folder>/<path>` at `size` metres, heights
// kept and so stretched (0379 point 6): from the store's entry when loaded,
// else from the file. A loaded entry is marked saved, its edits being in the
// file now, and loaded again from it at the next update, between frames,
// since a load may not run inside one. False, `why` naming the file and
// nothing written, when it will not read or write. `scratch` is rewound.
[[nodiscard]] bool voe_editor_models_landscape_size(voe_editor_models *models,
						    const char *folder,
						    const char *path, float size,
						    voe_base_arena *scratch,
						    voe_editor_notice *why);

// The store to read, for what draws, picks and outlines.
const voe_3d_models *voe_editor_models_store(const voe_editor_models *models);
