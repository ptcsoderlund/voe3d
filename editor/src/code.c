// A project's library loaded from a copy, as code.h says and why.
//
// Constraints: each folder on the way to loaded/ is listed to see it is
// there, then made one level; the listing and the copied bytes are rewound
// out of the caller's arena, so only the copy's path stays in it.
#include "code.h"

#include <base/assert.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define CODE_EXTENSION ".dll"
#else
#define CODE_EXTENSION ".so"
#endif

// "project-<load>" plus the extension, the largest uint32_t included.
#define CODE_NAME 32

// path already a folder, or made one level. False with why naming it.
static bool folder_ensure(const char *path, voe_base_arena *arena,
			  voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_platform_folder_listing listing;
	bool there;

	VOE_BASE_ASSERT(path != NULL, "making no folder");
	VOE_BASE_ASSERT(why != NULL, "making a folder with nowhere to say why");
	there = voe_platform_folder_list(path, arena, &listing, NULL);
	voe_base_arena_rewind(arena, mark);
	if (there)
		return true;
	voe_base_report_error_clear();
	if (voe_platform_folder_create(path, NULL))
		return true;
	voe_editor_notice_from_report(why, path);
	return false;
}

// folder's Build/editor/loaded/, made as needed, or NULL with why set.
static const char *loaded_folder(const char *folder, voe_base_arena *arena,
				 voe_editor_notice *why)
{
	const char *build = voe_platform_path_join(arena, folder, "Build");
	const char *editor = voe_platform_path_join(arena, build, "editor");
	const char *loaded = voe_platform_path_join(arena, editor, "loaded");

	VOE_BASE_ASSERT(folder != NULL, "loading code for no project");
	if (!folder_ensure(build, arena, why) ||
	    !folder_ensure(editor, arena, why) ||
	    !folder_ensure(loaded, arena, why))
		return NULL;
	VOE_BASE_ASSERT(loaded != NULL, "no loaded folder and no failure");
	return loaded;
}

// built's bytes written to copy, the bytes rewound out of arena after.
static bool file_copy(const char *built, const char *copy,
		      voe_base_arena *arena, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	const uint8_t *bytes;
	size_t count;
	bool written;

	VOE_BASE_ASSERT(built != NULL && copy != NULL, "copying no file");
	VOE_BASE_ASSERT(why != NULL, "copying a file with nowhere to say why");
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(built, arena, &count, NULL);
	if (bytes == NULL || count == 0) {
		if (bytes != NULL)
			voe_editor_notice_set(why, "%s is empty", built);
		else
			voe_editor_notice_from_report(why, built);
		voe_base_arena_rewind(arena, mark);
		return false;
	}
	voe_base_report_error_clear();
	written = voe_platform_file_write(copy, bytes, count, NULL);
	voe_base_arena_rewind(arena, mark);
	if (!written)
		voe_editor_notice_from_report(why, copy);
	return written;
}

bool voe_editor_code_open(const char *built, const char *folder, uint32_t load,
			  voe_base_arena *arena, voe_editor_code *out,
			  voe_editor_notice *why)
{
	char name[CODE_NAME];
	const char *loaded;
	const char *copy;
	voe_platform_library *library;
	voe_platform_symbol entry;

	VOE_BASE_ASSERT(built != NULL && folder != NULL, "loading no code");
	VOE_BASE_ASSERT(arena != NULL && out != NULL && why != NULL,
			"loading code with nowhere to put it");
	*out = (voe_editor_code){ 0 };
	loaded = loaded_folder(folder, arena, why);
	if (loaded == NULL)
		return false;
	snprintf(name, sizeof name, "project-%u" CODE_EXTENSION, load);
	copy = voe_platform_path_join(arena, loaded, name);
	if (!file_copy(built, copy, arena, why))
		return false;

	voe_base_report_error_clear();
	library = voe_platform_library_new(copy);
	if (library == NULL) {
		voe_editor_notice_from_report(why, copy);
		return false;
	}
	entry = voe_platform_library_symbol(library, "voe_game_project_register");
	if (entry == NULL) {
		voe_editor_notice_set(why, "%s has no voe_game_project_register",
				      copy);
		voe_platform_library_destroy(library);
		return false;
	}

	*out = (voe_editor_code){
		.library = library,
		.register_types = (void (*)(voe_ecs_world *))entry,
		.path = copy,
	};
	VOE_BASE_ASSERT(out->register_types != NULL, "code with no entry point");
	return true;
}

bool voe_editor_code_same(const char *built, const voe_editor_code *code,
			  voe_base_arena *arena)
{
	const uint8_t *fresh;
	const uint8_t *loaded;
	size_t fresh_count;
	size_t loaded_count;

	VOE_BASE_ASSERT(built != NULL && code != NULL, "comparing no code");
	VOE_BASE_ASSERT(arena != NULL, "comparing code with no arena");
	if (code->library == NULL)
		return false;
	fresh = voe_platform_file_read(built, arena, &fresh_count, NULL);
	loaded = voe_platform_file_read(code->path, arena, &loaded_count, NULL);
	return fresh != NULL && loaded != NULL && fresh_count == loaded_count &&
	       memcmp(fresh, loaded, fresh_count) == 0;
}

void voe_editor_code_close(voe_editor_code *code)
{
	VOE_BASE_ASSERT(code != NULL, "closing no code");

	if (code->library != NULL)
		voe_platform_library_destroy(code->library);
	*code = (voe_editor_code){ 0 };
	VOE_BASE_ASSERT(code->library == NULL, "closed code still open");
}
