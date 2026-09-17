// `<settings>/voe3d/last_project`: read as one line, written by making the two
// folders above it as needed. See the header for the shape of the contract.
#include "last_project.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <stdint.h>
#include <string.h>

// The name inside <settings> this engine's own settings live under, and the
// file inside that holding the one remembered path.
#define LAST_PROJECT_FOLDER "voe3d"
#define LAST_PROJECT_FILE "last_project"

// Scratch for the two folders' paths, the file's path, and the one line
// written into it. A block size, not a limit.
#define LAST_PROJECT_SCRATCH (4u * 1024u)

const char *voe_editor_last_project_read(voe_base_arena *arena)
{
	const char *settings;
	const char *dir;
	const char *path;
	const uint8_t *bytes;
	size_t size;
	char *text;
	char *newline;

	settings = voe_platform_folder_settings(arena);
	if (settings == NULL)
		return NULL;

	dir = voe_platform_path_join(arena, settings, LAST_PROJECT_FOLDER);
	path = voe_platform_path_join(arena, dir, LAST_PROJECT_FILE);

	if (!voe_platform_file_exists(path))
		return NULL;

	bytes = voe_platform_file_read(path, arena, &size, NULL);
	if (bytes == NULL)
		return NULL;

	// The file is NUL-terminated past its last byte whether or not it ends
	// in a newline (platform/file.h), so cutting at the first '\n' found
	// or leaving that terminator alone are the same call.
	text = (char *)bytes;
	newline = memchr(text, '\n', size);
	if (newline != NULL)
		*newline = '\0';
	return text;
}

// path already there, or made one level. False only when neither is true.
static bool ensure_folder(const char *path, voe_base_arena *scratch)
{
	voe_platform_folder_listing listing;
	voe_base_error error;

	if (voe_platform_folder_list(path, scratch, &listing, NULL))
		return true;
	return voe_platform_folder_create(path, &error);
}

bool voe_editor_last_project_write(const char *folder)
{
	voe_base_arena *scratch;
	const char *settings;
	const char *dir;
	const char *path;
	size_t length;
	char *line;
	bool ok;

	VOE_BASE_ASSERT(folder != NULL, "writing no folder as the last project");

	scratch = voe_base_arena_new(LAST_PROJECT_SCRATCH);

	settings = voe_platform_folder_settings(scratch);
	if (settings == NULL) {
		voe_base_arena_destroy(scratch);
		return false;
	}
	dir = voe_platform_path_join(scratch, settings, LAST_PROJECT_FOLDER);

	if (!ensure_folder(settings, scratch) || !ensure_folder(dir, scratch)) {
		voe_base_arena_destroy(scratch);
		return false;
	}

	path = voe_platform_path_join(scratch, dir, LAST_PROJECT_FILE);

	length = strlen(folder);
	line = voe_base_arena_push(scratch, length + 1);
	memcpy(line, folder, length);
	line[length] = '\n';

	ok = voe_platform_file_write(path, (const uint8_t *)line, length + 1,
				     NULL);
	voe_base_arena_destroy(scratch);
	return ok;
}
