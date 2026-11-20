// A tree step made with copies of its path and values, freed, a species' file
// written, and a step put back into the table, file and the open copy either
// way. See tree_steps.h for why it is its own memory.
#include "tree_steps.h"

#include <base/assert.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

voe_editor_tree_step *voe_editor_tree_step_new(const char *path,
					       const voe_assets_tree *before,
					       const voe_assets_tree *after)
{
	voe_editor_tree_step *step;
	size_t length;

	VOE_BASE_ASSERT(path != NULL && before != NULL && after != NULL,
			"a tree step on no path or with no values");
	length = strlen(path);
	VOE_BASE_ASSERT(length < VOE_ASSETS_LANDSCAPE_KIND_PATH,
			"a tree step's path too long");

	step = malloc(sizeof(*step));
	VOE_BASE_ASSERT(step != NULL, "out of memory making a tree step");
	memcpy(step->path, path, length + 1);
	step->before = *before;
	step->after = *after;
	return step;
}

void voe_editor_tree_step_destroy(voe_editor_tree_step *step)
{
	free(step);
}

bool voe_editor_tree_step_write(const char *folder, const char *path,
				const voe_assets_tree *values,
				voe_base_arena *scratch,
				voe_editor_disk_watch *watch,
				voe_base_error *error)
{
	struct voe_base_arena_mark mark;
	voe_assets_tree_text text;
	const char *full;
	bool written;

	VOE_BASE_ASSERT(folder != NULL && path != NULL && values != NULL,
			"writing a species into no folder, path or values");
	VOE_BASE_ASSERT(scratch != NULL && error != NULL,
			"writing a species with no scratch or error");
	mark = voe_base_arena_mark(scratch);
	text = voe_assets_tree_write(values, scratch);
	full = voe_platform_path_join(scratch, folder, path);
	written = voe_platform_file_write(full, (const uint8_t *)text.text,
					  text.size, error);
	if (written && watch != NULL)
		voe_editor_disk_watch_wrote(watch, full);
	voe_base_arena_rewind(scratch, mark);
	return written;
}

// Whether the table holds a row named `path`.
static bool tree_held(const voe_editor_trees *trees, const char *path)
{
	VOE_BASE_ASSERT(trees != NULL && path != NULL,
			"looking for a species in no table or of no path");
	VOE_BASE_ASSERT(trees->count <= VOE_EDITOR_TREES, "a table past its room");
	for (uint32_t i = 0; i < trees->count; i++)
		if (strcmp(trees->rows[i].path, path) == 0)
			return true;
	return false;
}

void voe_editor_tree_step_apply(const voe_editor_tree_step *step,
				voe_editor_models *models,
				voe_editor_scene *scene, const char *folder,
				voe_base_arena *scratch,
				voe_editor_disk_watch *watch, bool forward)
{
	const voe_assets_tree *values;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(step != NULL && models != NULL && scene != NULL,
			"applying no tree step, or to no store or scene");
	VOE_BASE_ASSERT(scratch != NULL, "applying a tree step with no scratch");
	values = forward ? &step->after : &step->before;
	if (folder == NULL ||
	    !tree_held(voe_editor_models_trees(models), step->path))
		return;
	voe_editor_models_tree_changed(models, step->path, values);
	if (!voe_editor_tree_step_write(folder, step->path, values, scratch,
					watch, &error))
		VOE_BASE_ERROR("editor", "could not save %s: %s", step->path,
			       voe_base_error_string(error));
	if (strcmp(scene->tree_open, step->path) == 0) {
		scene->tree = *values;
		scene->tree_before = *values;
	}
}
