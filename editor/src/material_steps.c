// A material step made with copies of its path and values, freed, a material's
// file written, and a step put back into the table, file, store and the open
// copy either way. See material_steps.h for why it is its own memory.
#include "material_steps.h"

#include "materials.h"

#include <base/assert.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

voe_editor_material_step *
voe_editor_material_step_new(const char *path,
			     const voe_assets_material_file *before,
			     const voe_assets_material_file *after)
{
	voe_editor_material_step *step;
	size_t length;

	VOE_BASE_ASSERT(path != NULL && before != NULL && after != NULL,
			"a material step on no path or with no values");
	length = strlen(path);
	VOE_BASE_ASSERT(length < VOE_ASSETS_MATERIAL_PATH,
			"a material step's path too long");

	step = malloc(sizeof(*step));
	VOE_BASE_ASSERT(step != NULL, "out of memory making a material step");
	memcpy(step->path, path, length + 1);
	step->before = *before;
	step->after = *after;
	return step;
}

void voe_editor_material_step_destroy(voe_editor_material_step *step)
{
	free(step);
}

bool voe_editor_material_step_write(const char *folder, const char *path,
				    const voe_assets_material_file *values,
				    voe_base_arena *scratch,
				    voe_base_error *error)
{
	struct voe_base_arena_mark mark;
	voe_assets_material_text text;
	bool written;

	VOE_BASE_ASSERT(folder != NULL && path != NULL && values != NULL,
			"writing a material into no folder, path or values");
	VOE_BASE_ASSERT(scratch != NULL && error != NULL,
			"writing a material with no scratch or error");
	mark = voe_base_arena_mark(scratch);
	text = voe_assets_material_write(values, scratch);
	written = voe_platform_file_write(
		voe_platform_path_join(scratch, folder, path),
		(const uint8_t *)text.text, text.size, error);
	voe_base_arena_rewind(scratch, mark);
	return written;
}

void voe_editor_material_step_apply(const voe_editor_material_step *step,
				    voe_editor_models *models,
				    voe_editor_scene *scene, const char *folder,
				    voe_base_arena *scratch, bool forward)
{
	const voe_assets_material_file *values;
	voe_base_error error = VOE_BASE_OK;
	voe_game_material *row;
	bool maps;

	VOE_BASE_ASSERT(step != NULL && models != NULL && scene != NULL,
			"applying no material step, or to no store or scene");
	VOE_BASE_ASSERT(scratch != NULL, "applying a material step with no scratch");
	values = forward ? &step->after : &step->before;
	row = voe_editor_materials_find(voe_editor_models_materials(models),
					step->path);
	if (row == NULL || folder == NULL)
		return;
	maps = strcmp(row->values.colormap, values->colormap) != 0 ||
	       strcmp(row->values.normalmap, values->normalmap) != 0 ||
	       strcmp(row->values.ormmap, values->ormmap) != 0;
	row->values = *values;
	if (!voe_editor_material_step_write(folder, step->path, values, scratch,
					    &error))
		VOE_BASE_ERROR("editor", "could not save %s: %s", step->path,
			       voe_base_error_string(error));
	voe_editor_models_material_changed(models, step->path, maps);
	if (strcmp(scene->material_open, step->path) == 0) {
		scene->material = *values;
		scene->material_before = *values;
	}
}
