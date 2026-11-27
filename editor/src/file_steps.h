// A file changed on disk taken into what the editor holds, and the two undo
// steps such a take settles into (0509 point 2): a file step for a file the
// editor writes at once, a landscape step for one saved with the project.
//
//     if (voe_editor_file_take(project, models, path, bytes, size, scratch,
//                              &why))
//             step = voe_editor_file_step_new(path, known, known_size,
//                                             bytes, size);
//     voe_editor_file_step_apply(step, project, models, watch, scratch, &why,
//                                false);                       // back
//     ground = voe_editor_landscape_step_new(path, held, taken);
//     voe_editor_landscape_step_apply(ground, models, false);   // back
//     voe_editor_file_step_destroy(step);
//
// THE KINDS TAKEN: a `.material` and a `.parts` read strictly and the tables
// read again; a `.prefab` that is not open checked strictly and the world's own
// text set again so its copies expand from the new file; `project.voe3d` read
// strictly and its values taken. The open scene or prefab is a scene text
// state, a landscape its own step, any other kind the caller's bug.
//
// A FILE STEP WRITES (0509 point 2). The editor writes these kinds at once, so
// they are never unsaved: stepping writes that side's bytes, tells the watch,
// and takes them in. It holds bytes, not values: the watch already holds the
// before as bytes, a prefab has no values short of a world, and the files are
// small. An empty side is not written, a write of no bytes being refused.
//
// A LANDSCAPE STEP WRITES NOTHING. A landscape is saved with the project, so a
// step puts its side into the store marked edited and leaves it unsaved, as a
// stroke does. It holds whole copies, not bytes: its file is megabytes to read
// back, a stroke is a rectangle and cannot carry layers, kinds and copies, and
// the store already holds the before.
//
// MEMORY, MALLOCED: rule 11's exception, for strokes.h's reason. A file step
// holds its path and both texts, a few KB. A landscape step holds two whole
// landscapes in one arena of its own: heights 4 and paint 8 bytes a point and
// its copies, some 6 MB at 512 cells and some 100 MB at 2048.
//
// Constraints: running out of memory is an assert. Needs a project with a
// folder. A landscape step's path must fit in VOE_3D_MODEL_PATH.
#pragma once

#include "models.h"
#include "project.h"

#include <3d/model_component.h>

#include <assets/landscape.h>

#include <base/arena.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The project-relative file at `path`, `size` bytes of `bytes` as on disk,
// taken in by its kind (see above). Writes nothing; `scratch` is rewound.
// False, `why` naming the file, line, key and why from the report, cleared
// first, with what the editor held kept.
[[nodiscard]] bool voe_editor_file_take(voe_editor_project *project,
					voe_editor_models *models,
					const char *path, const char *bytes,
					size_t size, voe_base_arena *scratch,
					voe_editor_notice *why);

// A project-relative path and its bytes before and after, each NUL past the
// last, which the sizes do not count; all the step's own.
typedef struct {
	char *path;
	uint8_t *before;
	size_t before_size;
	uint8_t *after;
	size_t after_size;
} voe_editor_file_step;

// A step holding copies of `path` and both sides; a side may be NULL when its
// size is 0. Never NULL; the caller destroys it or hands it to the undo line.
voe_editor_file_step *voe_editor_file_step_new(const char *path,
					       const uint8_t *before,
					       size_t before_size,
					       const uint8_t *after,
					       size_t after_size);

// Frees the step and its copies. NULL is nothing.
void voe_editor_file_step_destroy(voe_editor_file_step *step);

// The after bytes when `forward`, the before otherwise, written to
// `<folder>/<path>`, told to `watch` (NULL telling nothing), then taken in by
// voe_editor_file_take. False with `why` when the side is empty, the write is
// refused, or the take is. `scratch` is rewound.
[[nodiscard]] bool voe_editor_file_step_apply(const voe_editor_file_step *step,
					      voe_editor_project *project,
					      voe_editor_models *models,
					      voe_editor_disk_watch *watch,
					      voe_base_arena *scratch,
					      voe_editor_notice *why,
					      bool forward);

// A landscape's path and whole landscapes before and after, in `memory`.
typedef struct {
	char path[VOE_3D_MODEL_PATH];
	voe_base_arena *memory;
	voe_assets_landscape before;
	voe_assets_landscape after;
} voe_editor_landscape_step;

// A step holding copies of `path`, `before` and `after`, each an exact
// resample at its own cells. Never NULL.
voe_editor_landscape_step *
voe_editor_landscape_step_new(const char *path,
			      const voe_assets_landscape *before,
			      const voe_assets_landscape *after);

// Frees the step and its landscapes. NULL is nothing.
void voe_editor_landscape_step_destroy(voe_editor_landscape_step *step);

// The after landscape when `forward`, the before otherwise, taken into the
// store by voe_editor_models_landscape_take, edited.
void voe_editor_landscape_step_apply(const voe_editor_landscape_step *step,
				     voe_editor_models *models, bool forward);
