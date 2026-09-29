// A tree made a prefab: the refusals, the file written, then the rows queued
// (prefabs.h).
//
// Constraints: a tree is read up to VOE_EDITOR_SCENE_ROWS entities, the
// identity table's size. The rows go through the structural queue one by one,
// and it has no undo: a queue that fills partway leaves the rows before it
// queued. It holds 2048 requests and a tree adds at most 33, so that takes a
// frame already queueing nearly two thousand; a room query on the queue would
// lift it.
#include "prefabs.h"

#include "scene.h"

#include <authoring/prefab.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <platform/file.h>
#include <platform/path.h>

#include <scene/camera_component.h>
#include <scene/identity_component.h>
#include <scene/light_component.h>
#include <scene/parent_component.h>
#include <scene/prefab_component.h>

#include <stdio.h>
#include <string.h>

bool voe_editor_prefab_refused(const voe_ecs_world *world, voe_ecs_entity root,
			       voe_editor_notice *why)
{
	voe_ecs_entity tree[VOE_EDITOR_SCENE_ROWS];
	uint32_t count;

	VOE_BASE_ASSERT(world != NULL && why != NULL,
			"checking a prefab with no world or nowhere to say why");
	count = voe_scene_parent_tree(world, root, tree, VOE_EDITOR_SCENE_ROWS);
	for (uint32_t i = 0; i < count; i++) {
		const char *what = NULL;

		if (voe_scene_camera_get(world, tree[i]) != NULL)
			what = "a camera";
		else if (voe_scene_light_get(world, tree[i]) != NULL)
			what = "a light";
		else if (voe_scene_prefab_get(world, tree[i]) != NULL ||
			 voe_scene_prefab_part_get(world, tree[i]) != NULL)
			what = "another prefab";
		if (what != NULL) {
			voe_editor_notice_set(why, "A prefab cannot hold %s.",
					      what);
			return true;
		}
	}
	return false;
}

// `Assets/<shown>/<name>.prefab` into `path`, `\` made `/`. False when it does
// not fit a prefab row.
static bool path_make(char path[VOE_SCENE_PREFAB_PATH], const char *shown,
		      const char *name)
{
	int length = shown[0] == '\0' ?
			     snprintf(path, VOE_SCENE_PREFAB_PATH,
				      "Assets/%s.prefab", name) :
			     snprintf(path, VOE_SCENE_PREFAB_PATH,
				      "Assets/%s/%s.prefab", shown, name);

	if (length < 0 || length >= VOE_SCENE_PREFAB_PATH)
		return false;
	for (int i = 0; i < length; i++)
		if (path[i] == '\\')
			path[i] = '/';
	return true;
}

// The prefab row on the root, then a part row naming it on every one of the
// tree. False at the first the queue refuses.
static bool rows_queue(voe_ecs_world *world, voe_ecs_entity root,
		       const char *path)
{
	voe_ecs_entity tree[VOE_EDITOR_SCENE_ROWS];
	uint32_t count =
		voe_scene_parent_tree(world, root, tree, VOE_EDITOR_SCENE_ROWS);
	voe_scene_prefab prefab = { 0 };
	voe_scene_prefab_part part = { .instance = root };
	voe_ecs_type part_type =
		voe_ecs_component_type(world, &voe_scene_prefab_part_key);

	snprintf(prefab.path, sizeof prefab.path, "%s", path);
	if (!voe_ecs_structure_add(
		    world, voe_ecs_component_type(world, &voe_scene_prefab_key),
		    root, &prefab))
		return false;
	for (uint32_t i = 0; i < count; i++)
		if (!voe_ecs_structure_add(world, part_type, tree[i], &part))
			return false;
	return true;
}

bool voe_editor_prefab_make(voe_editor_project *project, voe_ecs_entity root,
			    const char *shown, voe_base_arena *scratch,
			    voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	const voe_scene_identity *identity;
	char path[VOE_SCENE_PREFAB_PATH];
	voe_authoring_text text;
	voe_base_error error;
	const char *absolute;
	bool made = false;

	VOE_BASE_ASSERT(project != NULL && scratch != NULL && why != NULL,
			"making a prefab with no project, scratch or notice");
	if (project->folder == NULL) {
		voe_editor_notice_set(why,
				      "Save the project before making a prefab.");
		return false;
	}
	if (project->prefab[0] != '\0') {
		voe_editor_notice_set(why,
				      "Go back to the level before making a prefab.");
		return false;
	}
	identity = voe_scene_identity_get(project->world, root);
	if (identity == NULL) {
		voe_editor_notice_set(why, "Only a named entity makes a prefab.");
		return false;
	}
	if (voe_editor_prefab_refused(project->world, root, why))
		return false;
	if (!path_make(path, shown != NULL ? shown : "", identity->name)) {
		voe_editor_notice_set(why, "The prefab's path is too long.");
		return false;
	}

	mark = voe_base_arena_mark(scratch);
	absolute = voe_platform_path_join(scratch, project->folder, path);
	voe_base_report_error_clear();
	if (voe_platform_file_exists(absolute)) {
		voe_editor_notice_set(why, "%s already exists.", path);
	} else if (!voe_authoring_prefab_write(project->world, root, scratch,
					       &text)) {
		voe_editor_notice_from_report(why, path);
	} else if (!voe_platform_file_write(absolute,
					    (const uint8_t *)text.text,
					    text.size, &error)) {
		voe_editor_notice_set(why, "Could not write %s: %s", path,
				      voe_base_error_string(error));
	} else if (!rows_queue(project->world, root, path)) {
		voe_editor_notice_set(why, "The scene is full.");
	} else {
		made = true;
	}
	voe_base_arena_rewind(scratch, mark);
	return made;
}
