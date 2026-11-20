// A prefab opened in the level's place, saved, and Back to the level: the
// three calls project.h's PREFAB OPENED IN PLACE OF THE LEVEL point describes,
// split out of project.c by function. Open and Back are project.h's; the save
// is project_prefab.h's, called by voe_editor_project_save alone.
//
// Every world change goes through voe_editor_project_scene_set, so the level
// set aside and read back is expanded from the prefab files as they are then.
// The level's text lives in an arena of its own, made by open and destroyed by
// Back or voe_editor_project_destroy.
#include "project.h"
#include "project_prefab.h"

#include "prefabs.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <scene/identity_system.h>
#include <scene/parent_component.h>

#include <stdint.h>
#include <string.h>

// The arena the set-aside level lives in and the scratch a prefab's save text
// is built in: project.c's save scratch, a block size, not a limit.
#define PREFAB_ARENA (1u * 1024u * 1024u)

bool voe_editor_project_prefab_save(voe_editor_project *project,
				    voe_editor_disk_watch *watch,
				    voe_editor_notice *why)
{
	const voe_ecs_entity *authored = voe_scene_identity_entities(project->world);
	uint32_t count = voe_scene_identity_count(project->world);
	voe_ecs_entity root = { 0 };
	uint32_t roots = 0;
	voe_base_arena *scratch;
	voe_base_error error;
	const char *path;
	voe_authoring_text text;
	bool saved = false;

	VOE_BASE_ASSERT(project->folder != NULL && project->prefab[0] != '\0',
			"saving a prefab with no prefab open");
	for (uint32_t i = 0; i < count; i++) {
		if (voe_scene_parent_get(project->world, authored[i]) == NULL) {
			root = authored[i];
			roots++;
		}
	}
	if (roots != 1) {
		voe_editor_notice_set(
			why, "A prefab holds exactly one entity with no parent.");
		return false;
	}
	if (voe_editor_prefab_refused(project->world, root, why))
		return false;

	scratch = voe_base_arena_new(PREFAB_ARENA);
	path = voe_platform_path_join(scratch, project->folder, project->prefab);
	voe_base_report_error_clear();
	if (!voe_editor_project_scene_text(project, scratch, &text) ||
	    !voe_platform_file_write(path, (const uint8_t *)text.text,
				     text.size, &error)) {
		voe_editor_notice_from_report(why, path);
	} else {
		if (watch != NULL)
			voe_editor_disk_watch_wrote(watch, path);
		project->unsaved = false;
		saved = true;
	}
	voe_base_arena_destroy(scratch);
	return saved;
}

bool voe_editor_project_prefab_open(voe_editor_project *project,
				    const char *path, voe_editor_notice *why)
{
	voe_base_arena *arena;
	voe_base_error error;
	voe_authoring_text level;
	const char *absolute;
	const uint8_t *bytes;
	size_t size;
	voe_editor_notice ignored;

	VOE_BASE_ASSERT(project != NULL && path != NULL && why != NULL,
			"opening a prefab with no project, path or notice");
	VOE_BASE_ASSERT(project->folder != NULL, "opening a prefab untitled");
	VOE_BASE_ASSERT(project->prefab[0] == '\0' &&
				project->level_arena == NULL,
			"opening a prefab while one is open");

	if (strlen(path) >= sizeof project->prefab) {
		voe_editor_notice_set(why, "The prefab's path is too long.");
		return false;
	}
	arena = voe_base_arena_new(PREFAB_ARENA);
	voe_base_report_error_clear();
	if (!voe_editor_project_scene_text(project, arena, &level)) {
		voe_editor_notice_from_report(why, "the scene");
		voe_base_arena_destroy(arena);
		return false;
	}
	absolute = voe_platform_path_join(arena, project->folder, path);
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(absolute, arena, &size, &error);
	if (bytes == NULL) {
		voe_editor_notice_from_report(why, path);
		voe_base_arena_destroy(arena);
		return false;
	}
	if (!voe_editor_project_scene_set(project, (const char *)bytes, size,
					  why)) {
		// The world is half-loaded; the level's own text always fits
		// back into the world it came from (scene_set).
		bool restored = voe_editor_project_scene_set(
			project, level.text, level.size, &ignored);

		VOE_BASE_ASSERT(restored,
				"the level did not read back after a refused prefab");
		voe_base_arena_destroy(arena);
		return false;
	}

	memcpy(project->prefab, path, strlen(path) + 1);
	project->level_arena = arena;
	project->level_text = level.text;
	project->level_size = level.size;
	project->level_unsaved = project->unsaved;
	project->unsaved = false;
	VOE_BASE_ASSERT(project->prefab[0] != '\0', "an open prefab with no path");
	return true;
}

bool voe_editor_project_prefab_back(voe_editor_project *project,
				    voe_editor_notice *why)
{
	VOE_BASE_ASSERT(project != NULL && why != NULL,
			"going back from no project or with nowhere to say why");
	VOE_BASE_ASSERT(project->prefab[0] != '\0' &&
				project->level_arena != NULL,
			"going back with no prefab open");

	if (!voe_editor_project_scene_set(project, project->level_text,
					  project->level_size, why))
		return false;
	project->unsaved = project->level_unsaved;
	voe_base_arena_destroy(project->level_arena);
	project->level_arena = NULL;
	project->level_text = NULL;
	project->level_size = 0;
	project->prefab[0] = '\0';
	VOE_BASE_ASSERT(project->level_arena == NULL, "a level left set aside");
	return true;
}
