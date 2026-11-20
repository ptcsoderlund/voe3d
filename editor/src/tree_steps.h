// One species edit as an undo step carries it (0515 point 6), as a material
// step does (material_steps.h): which `.tree`, and its values before and after,
// so stepping over it puts one or the other back into the table, the file and
// the open copy.
//
//     step = voe_editor_tree_step_new(path, &before, &after);
//     voe_editor_undo_tree(&undo, project, scratch, step);   // undo owns it
//     voe_editor_tree_step_apply(step, models, scene, folder, scratch,
//                                watch, false);              // back
//     voe_editor_tree_step_destroy(step);
//
// A STEP RIDES BESIDE THE SCENE TEXT, NOT IN IT: a species is a file of its
// own, and the species world's text names it only by its variation's path.
//
// ITS OWN MEMORY, MALLOCED, for strokes.h's reason: a line of 64 states may
// hold one or none. Freed when its state is dropped.
//
// A STEP WRITES THE FILE AT ONCE, not at Save (0515 point 6): Save writes no
// world while a species is open, so the file must already hold the values
// shown. The table takes them through voe_editor_models_tree_changed, which
// marks the row to regrow. A path the table no longer holds (trashed, or
// renamed) is left alone, so a step never makes a file that was taken away.
//
// Constraints: running out of memory is an assert. `path` must fit in
// VOE_ASSETS_LANDSCAPE_KIND_PATH with its terminator, as a table row's path
// does. A write refused while stepping is said on stderr, the step having no
// notice.
#pragma once

#include "disk_watch.h"
#include "models.h"
#include "scene.h"
#include "trees.h"

#include <assets/tree.h>

#include <base/arena.h>
#include <base/error.h>

#include <stdbool.h>

// A `.tree`'s `Assets/...` path, and its values as they were and as the edit
// left them.
typedef struct {
	char path[VOE_ASSETS_LANDSCAPE_KIND_PATH];
	voe_assets_tree before;
	voe_assets_tree after;
} voe_editor_tree_step;

// A step holding copies of `path`, `before` and `after`. Never NULL; the caller
// destroys it or hands it to the undo line.
voe_editor_tree_step *voe_editor_tree_step_new(const char *path,
					       const voe_assets_tree *before,
					       const voe_assets_tree *after);

// Frees the step. NULL is nothing.
void voe_editor_tree_step_destroy(voe_editor_tree_step *step);

// `values` written as `<folder>/<path>`'s text through `platform`, and the
// write told to `watch` (disk_watch.h), NULL telling nothing. False with
// `error` filled when the write is refused. `scratch` is rewound.
[[nodiscard]] bool voe_editor_tree_step_write(const char *folder,
					      const char *path,
					      const voe_assets_tree *values,
					      voe_base_arena *scratch,
					      voe_editor_disk_watch *watch,
					      voe_base_error *error);

// `after` when `forward`, `before` otherwise, taken into the table's row
// through voe_editor_models_tree_changed, written to `<folder>/<path>` and
// told to `watch` as _write tells it and, when the path is the open one, into
// `scene->tree` and its `tree_before`. Nothing when the table holds no such
// row or `folder` is NULL. `scratch` is rewound.
void voe_editor_tree_step_apply(const voe_editor_tree_step *step,
				voe_editor_models *models,
				voe_editor_scene *scene, const char *folder,
				voe_base_arena *scratch,
				voe_editor_disk_watch *watch, bool forward);
