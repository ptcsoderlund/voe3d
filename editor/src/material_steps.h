// One material edit as an undo step carries it (0399 point 9): which
// `.material`, and its values before and after, so stepping over it puts one
// or the other back into the table, the file and the store.
//
//     step = voe_editor_material_step_new(path, &before, &after);
//     voe_editor_undo_material(&undo, project, scratch, step); // undo owns it
//     voe_editor_material_step_apply(step, models, scene, folder, scratch,
//                                    false);                  // back
//     voe_editor_material_step_destroy(step);
//
// A STEP RIDES BESIDE THE SCENE TEXT, NOT IN IT, as a stroke does (strokes.h):
// a material is a file of its own, which the scene text names only by path.
//
// ITS OWN MEMORY, MALLOCED, for strokes.h's reason: a line of 64 states may
// hold one or none, so room for the worst out of an arena would be held by
// every editor that never edits a material. Freed when its state is dropped.
//
// A STEP WRITES THE FILE AT ONCE, not at Save (0399 point 9): a Duplicate
// copies the file on disk, so the file must already hold the values shown.
// A path the table no longer holds (trashed, or renamed without the line
// forgotten) is left alone, so a step never makes a file that was taken away.
//
// Constraints: running out of memory is an assert. `path` must fit in
// VOE_ASSETS_MATERIAL_PATH with its terminator, as a table row's path does. A
// write refused while stepping is said on stderr, the step having no notice.
#pragma once

#include "models.h"
#include "scene.h"

#include <assets/material.h>

#include <base/arena.h>
#include <base/error.h>

#include <stdbool.h>

// A `.material`'s `Assets/...` path, and its values as they were and as the
// edit left them.
typedef struct {
	char path[VOE_ASSETS_MATERIAL_PATH];
	voe_assets_material_file before;
	voe_assets_material_file after;
} voe_editor_material_step;

// A step holding copies of `path`, `before` and `after`. Never NULL; the caller
// destroys it or hands it to the undo line.
voe_editor_material_step *
voe_editor_material_step_new(const char *path,
			     const voe_assets_material_file *before,
			     const voe_assets_material_file *after);

// Frees the step. NULL is nothing.
void voe_editor_material_step_destroy(voe_editor_material_step *step);

// `values` written as `<folder>/<path>`'s text through `platform`. False with
// `error` filled when the write is refused. `scratch` is rewound.
[[nodiscard]] bool
voe_editor_material_step_write(const char *folder, const char *path,
			       const voe_assets_material_file *values,
			       voe_base_arena *scratch, voe_base_error *error);

// `after` when `forward`, `before` otherwise, taken into the table's row,
// written to `<folder>/<path>`, told to the store (`maps` when a map path
// differs) and, when the path is the open one, into `scene->material` and its
// `material_before`. Nothing when the table holds no such row or `folder` is
// NULL. `scratch` is rewound.
void voe_editor_material_step_apply(const voe_editor_material_step *step,
				    voe_editor_models *models,
				    voe_editor_scene *scene, const char *folder,
				    voe_base_arena *scratch, bool forward);
