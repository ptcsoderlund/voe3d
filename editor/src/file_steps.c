// A changed file taken in by its kind, and the file and landscape steps made
// with their own copies, freed, and applied either way. See file_steps.h for
// why each holds what it does and why it is its own memory.
#include "file_steps.h"

#include "assets_panel.h"

#include <assets/material.h>
#include <assets/parts.h>

#include <authoring/project.h>
#include <authoring/scene_read.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdlib.h>
#include <string.h>

// The block size of a landscape step's arena: a default, not a limit.
#define LANDSCAPE_STEP_ARENA (1u * 1024u * 1024u)

// A `.material` or `.parts` read strictly, then the tables read again.
static bool take_material(voe_editor_project *project,
			  voe_editor_models *models, const char *path,
			  const char *bytes, size_t size,
			  voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_assets_material_file *material;
	voe_assets_parts *parts;
	bool read;

	VOE_BASE_ASSERT(project->folder != NULL && models != NULL,
			"taking a material into no folder or store");
	if (voe_editor_assets_name_ends(path, ".material")) {
		material = voe_base_arena_push(scratch, sizeof(*material));
		read = voe_assets_material_read_strict(bytes, size, material,
						       NULL);
	} else {
		parts = voe_base_arena_push(scratch, sizeof(*parts));
		read = voe_assets_parts_read(bytes, size, parts, NULL);
	}
	voe_base_arena_rewind(scratch, mark);
	if (!read) {
		voe_editor_notice_from_report(why, path);
		return false;
	}
	voe_editor_models_materials_read(models, project->folder, scratch);
	return true;
}

// A closed prefab checked strictly, then the world's own text set again so
// its copies expand from the new file; unsaved put back as it was.
static bool take_prefab(voe_editor_project *project, const char *path,
			const char *bytes, size_t size,
			voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_authoring_text text;
	bool unsaved = project->unsaved;
	bool taken;

	VOE_BASE_ASSERT(strcmp(project->prefab, path) != 0,
			"taking the open prefab in as a closed one");
	if (!voe_authoring_scene_check(bytes, size, project->world, scratch)) {
		voe_base_arena_rewind(scratch, mark);
		voe_editor_notice_from_report(why, path);
		return false;
	}
	voe_base_arena_rewind(scratch, mark);
	voe_base_report_error_clear();
	if (!voe_editor_project_scene_text(project, scratch, &text)) {
		voe_base_arena_rewind(scratch, mark);
		voe_editor_notice_from_report(why, "the scene");
		return false;
	}
	taken = voe_editor_project_scene_set(project, text.text, text.size, why);
	voe_base_arena_rewind(scratch, mark);
	project->unsaved = unsaved;
	return taken;
}

// project.voe3d read strictly, then its values taken.
static bool take_project_file(voe_editor_project *project, const char *path,
			      const char *bytes, size_t size,
			      voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_authoring_project file;
	bool read;

	VOE_BASE_ASSERT(project->folder != NULL,
			"taking a project file into an untitled project");
	read = voe_authoring_project_read_strict(bytes, size, scratch, &file);
	if (read)
		voe_editor_project_file_take(project, &file);
	else
		voe_editor_notice_from_report(why, path);
	voe_base_arena_rewind(scratch, mark);
	return read;
}

bool voe_editor_file_take(voe_editor_project *project,
			  voe_editor_models *models, const char *path,
			  const char *bytes, size_t size,
			  voe_base_arena *scratch, voe_editor_notice *why)
{
	VOE_BASE_ASSERT(project != NULL && project->folder != NULL &&
				models != NULL,
			"taking a file into no project, folder or store");
	VOE_BASE_ASSERT(path != NULL && bytes != NULL && scratch != NULL &&
				why != NULL,
			"taking in no path or bytes, or with no scratch or why");
	voe_base_report_error_clear();
	if (voe_editor_assets_name_ends(path, ".material") ||
	    voe_editor_assets_name_ends(path, ".parts"))
		return take_material(project, models, path, bytes, size,
				     scratch, why);
	if (voe_editor_assets_name_ends(path, ".prefab"))
		return take_prefab(project, path, bytes, size, scratch, why);
	VOE_BASE_ASSERT(strcmp(path, VOE_AUTHORING_PROJECT_FILE) == 0,
			"taking in a file of a kind no file step holds");
	return take_project_file(project, path, bytes, size, scratch, why);
}

// `size` bytes of `bytes` copied to `at` with a NUL past the last.
static uint8_t *copy_bytes(uint8_t *at, const uint8_t *bytes, size_t size)
{
	VOE_BASE_ASSERT(at != NULL && (bytes != NULL || size == 0),
			"copying bytes to nowhere or from nothing");
	if (size > 0)
		memcpy(at, bytes, size);
	at[size] = '\0';
	return at;
}

voe_editor_file_step *voe_editor_file_step_new(const char *path,
					       const uint8_t *before,
					       size_t before_size,
					       const uint8_t *after,
					       size_t after_size)
{
	voe_editor_file_step *step;
	size_t length;
	uint8_t *at;

	VOE_BASE_ASSERT(path != NULL, "a file step on no path");
	length = strlen(path) + 1;
	step = malloc(sizeof(*step) + length + before_size + after_size + 2);
	VOE_BASE_ASSERT(step != NULL, "out of memory making a file step");
	at = (uint8_t *)(step + 1);
	step->path = memcpy(at, path, length);
	step->before = copy_bytes(at + length, before, before_size);
	step->before_size = before_size;
	step->after = copy_bytes(step->before + before_size + 1, after,
				 after_size);
	step->after_size = after_size;
	return step;
}

void voe_editor_file_step_destroy(voe_editor_file_step *step)
{
	free(step);
}

bool voe_editor_file_step_apply(const voe_editor_file_step *step,
				voe_editor_project *project,
				voe_editor_models *models,
				voe_editor_disk_watch *watch,
				voe_base_arena *scratch,
				voe_editor_notice *why, bool forward)
{
	struct voe_base_arena_mark mark;
	voe_base_error error = VOE_BASE_OK;
	const uint8_t *bytes;
	size_t size;
	const char *full;
	bool written;

	VOE_BASE_ASSERT(step != NULL && project != NULL &&
				project->folder != NULL,
			"applying no file step, or to no project folder");
	VOE_BASE_ASSERT(scratch != NULL && why != NULL,
			"applying a file step with no scratch or why");
	bytes = forward ? step->after : step->before;
	size = forward ? step->after_size : step->before_size;
	if (size == 0) {
		voe_editor_notice_set(why, "%s: left as it is, that side empty",
				      step->path);
		return false;
	}
	mark = voe_base_arena_mark(scratch);
	full = voe_platform_path_join(scratch, project->folder, step->path);
	written = voe_platform_file_write(full, bytes, size, &error);
	if (written && watch != NULL)
		voe_editor_disk_watch_wrote(watch, full);
	voe_base_arena_rewind(scratch, mark);
	if (!written) {
		voe_editor_notice_set(why, "%s: could not be written: %s",
				      step->path, voe_base_error_string(error));
		return false;
	}
	return voe_editor_file_take(project, models, step->path,
				    (const char *)bytes, size, scratch, why);
}

voe_editor_landscape_step *
voe_editor_landscape_step_new(const char *path,
			      const voe_assets_landscape *before,
			      const voe_assets_landscape *after)
{
	voe_editor_landscape_step *step;
	size_t length;

	VOE_BASE_ASSERT(path != NULL && before != NULL && after != NULL,
			"a landscape step on no path or with no landscapes");
	length = strlen(path);
	VOE_BASE_ASSERT(length < VOE_3D_MODEL_PATH,
			"a landscape step's path too long");
	step = malloc(sizeof(*step));
	VOE_BASE_ASSERT(step != NULL, "out of memory making a landscape step");
	memcpy(step->path, path, length + 1);
	step->memory = voe_base_arena_new(LANDSCAPE_STEP_ARENA);
	step->before = voe_assets_landscape_resample(before, before->cells,
						     step->memory);
	step->after = voe_assets_landscape_resample(after, after->cells,
						    step->memory);
	return step;
}

void voe_editor_landscape_step_destroy(voe_editor_landscape_step *step)
{
	if (step == NULL)
		return;
	voe_base_arena_destroy(step->memory);
	free(step);
}

void voe_editor_landscape_step_apply(const voe_editor_landscape_step *step,
				     voe_editor_models *models, bool forward)
{
	VOE_BASE_ASSERT(step != NULL && models != NULL,
			"applying no landscape step, or to no store");
	voe_editor_models_landscape_take(models, step->path,
					 forward ? &step->after : &step->before,
					 true);
}
