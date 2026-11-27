// The check: the folder found, the world made with the project's code when it
// is built, every listed file read by its kind's strict reader, and the lines.
// See check.h for what is read and what is said.
#include "check.h"

#include "assets_panel.h"
#include "assets_walk.h"
#include "code.h"
#include "game_tree.h"
#include "notice.h"
#include "prefabs.h"

#include <assets/landscape.h>
#include <assets/material.h>
#include <assets/parts.h>

#include <authoring/project.h>
#include <authoring/scene_read.h>

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <game/world.h>

#include <platform/file.h>
#include <platform/path.h>

#include <scene/identity_component.h>
#include <scene/parent_component.h>

#include <stdio.h>
#include <string.h>

// The block size of the check's one arena: a default, not a limit.
#define CHECK_ARENA (4u * 1024u * 1024u)

// A world with game/world.h's types, then code's own when it has a library.
static voe_ecs_world *world_make(voe_base_arena *arena,
				 const voe_editor_code *code)
{
	voe_ecs_world *world = voe_game_world_new(arena);

	VOE_BASE_ASSERT(world != NULL && code != NULL,
			"no world made, or no code to ask");
	if (code->library != NULL)
		code->register_types(world);
	return world;
}

// The library in Build/editor/ loaded into code when there is one; a line on
// stdout when it will not load, or when Code/ has no library built.
static void code_load(const char *folder, voe_base_arena *arena,
		      voe_editor_code *code)
{
	const char *built = voe_editor_game_tree_library(folder, arena);
	voe_editor_notice why = { 0 };

	VOE_BASE_ASSERT(code != NULL && code->library == NULL,
			"loading code over code already loaded");
	if (voe_platform_file_exists(built)) {
		if (!voe_editor_code_open(built, folder, 0, arena, code, &why))
			printf("%s; the project's own components were not checked\n",
			       why.text);
	} else if (voe_editor_game_tree_has_code(folder, arena)) {
		printf("Code/ has no library built in Build/editor/, so the project's own components were not checked\n");
	}
	VOE_BASE_ASSERT(code->library == NULL || code->register_types != NULL,
			"code loaded with no entry point");
}

// A prefab's text read into a fresh world: false, reported, when it will not
// read; false with why unless exactly one entity has no parent and
// voe_editor_prefab_refused lets its tree through.
static bool prefab_takes(const char *text, size_t size,
			 const voe_editor_code *code, voe_base_arena *arena,
			 voe_editor_notice *why)
{
	voe_ecs_world *world = world_make(arena, code);
	voe_authoring_kept kept;
	const voe_ecs_entity *authored;
	uint32_t count;
	voe_ecs_entity root = { 0 };
	uint32_t roots = 0;

	if (!voe_authoring_scene_read(text, size, world, arena, &kept))
		return false;
	authored = voe_scene_identity_entities(world);
	count = voe_scene_identity_count(world);
	for (uint32_t i = 0; i < count; i++) {
		if (voe_scene_parent_get(world, authored[i]) == NULL) {
			root = authored[i];
			roots++;
		}
	}
	if (roots != 1) {
		voe_editor_notice_set(
			why, "A prefab holds exactly one entity with no parent.");
		return false;
	}
	return !voe_editor_prefab_refused(world, root, why);
}

// path's text handed to its kind's strict reader. False, reported, or with why
// set when the refusal is the check's own.
static bool file_takes(const char *path, const char *text, size_t size,
		       voe_ecs_world *world, const voe_editor_code *code,
		       voe_base_arena *arena, voe_editor_notice *why)
{
	voe_authoring_project project;

	VOE_BASE_ASSERT(path != NULL && world != NULL && why != NULL,
			"checking no path, or with no world or why");
	if (voe_editor_assets_name_ends(path, ".scene"))
		return voe_authoring_scene_check(text, size, world, arena);
	if (voe_editor_assets_name_ends(path, ".prefab"))
		return voe_authoring_scene_check(text, size, world, arena) &&
		       prefab_takes(text, size, code, arena, why);
	if (voe_editor_assets_name_ends(path, ".material"))
		return voe_assets_material_read_strict(
			text, size,
			voe_base_arena_push(arena, sizeof(voe_assets_material_file)),
			NULL);
	if (voe_editor_assets_name_ends(path, ".landscape"))
		return voe_assets_landscape_read_strict(
			text, size, arena,
			voe_base_arena_push(arena, sizeof(voe_assets_landscape)),
			NULL);
	if (voe_editor_assets_name_ends(path, ".parts"))
		return voe_assets_parts_read(
			text, size, voe_base_arena_push(arena, sizeof(voe_assets_parts)),
			NULL);
	VOE_BASE_ASSERT(strcmp(path, VOE_AUTHORING_PROJECT_FILE) == 0,
			"checking a file of a kind the list does not hold");
	return voe_authoring_project_read_strict(text, size, arena, &project);
}

// One listed file read and checked, its `<path>: <why>` line written when
// refused; the arena rewound either way.
static bool file_check(const char *folder, const char *path,
		       voe_ecs_world *world, const voe_editor_code *code,
		       voe_base_arena *arena)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_base_error error = VOE_BASE_OK;
	voe_editor_notice why = { 0 };
	voe_editor_notice line = { 0 };
	const uint8_t *bytes;
	size_t size = 0;
	bool taken;

	VOE_BASE_ASSERT(folder != NULL && path != NULL,
			"checking a file in no folder or at no path");
	voe_base_report_error_clear();
	voe_base_report_subject_push(path);
	bytes = voe_platform_file_read(voe_platform_path_join(arena, folder, path),
				       arena, &size, &error);
	taken = bytes != NULL && file_takes(path, (const char *)bytes, size,
					    world, code, arena, &why);
	voe_base_report_subject_pop();
	if (!taken) {
		if (why.text[0] != '\0')
			voe_editor_notice_set(&line, "%s: %s", path, why.text);
		else
			voe_editor_notice_from_report(&line, path);
		printf("%s\n", line.text);
	}
	voe_base_arena_rewind(arena, mark);
	return taken;
}

int voe_editor_check(const char *folder)
{
	voe_base_arena *arena = voe_base_arena_new(CHECK_ARENA);
	voe_base_error error = VOE_BASE_OK;
	voe_editor_code code = { 0 };
	voe_editor_notice why = { 0 };
	voe_editor_assets_files files = { 0 };
	voe_ecs_world *world;
	uint32_t refused = 0;
	int status = 1;

	if (folder == NULL)
		folder = voe_platform_path_absolute(".", arena, &error);
	if (folder == NULL ||
	    !voe_platform_file_exists(voe_platform_path_join(
		    arena, folder, VOE_AUTHORING_PROJECT_FILE))) {
		printf("%s is not a project: it holds no %s\n",
		       folder != NULL ? folder : ".", VOE_AUTHORING_PROJECT_FILE);
		goto destroy;
	}
	code_load(folder, arena, &code);
	world = world_make(arena, &code);
	if (!voe_editor_assets_list(folder, arena, &files, &why)) {
		printf("%s\n", why.text);
		goto destroy;
	}
	for (uint32_t i = 0; i < files.count; i++)
		refused += file_check(folder, files.files[i].path, world, &code,
				      arena) ?
				   0 :
				   1;
	if (refused == 0)
		printf("all is well: %u files checked\n", files.count);
	else
		printf("%u of %u files refused\n", refused, files.count);
	status = refused == 0 ? 0 : 1;

destroy:
	// The worlds go with the arena before the code they hold keys of.
	voe_base_arena_destroy(arena);
	voe_editor_code_close(&code);
	VOE_BASE_ASSERT(status == 0 || status == 1, "a check exit beside 0 and 1");
	return status;
}
