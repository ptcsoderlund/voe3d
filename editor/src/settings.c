// `<settings>/voe3d/editor_settings`: read line by line as a key and a number,
// written back with every other key's line kept, by making the two folders
// above it as needed. See the header for the shape of the contract.
#include "settings.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The name inside <settings> this engine's own settings live under, and the
// file inside that holding the editor's.
#define SETTINGS_FOLDER "voe3d"
#define SETTINGS_FILE "editor_settings"

// Scratch for the two folders' paths, the file's path and its text. A block
// size, not a limit.
#define SETTINGS_SCRATCH (4u * 1024u)

// The largest size a line may hold, in millimetres.
#define SETTINGS_MOST 1000.0f

// Room for the three lines this file writes: each key and any finite float
// `%.3f` prints.
#define SETTINGS_OWN_ROOM 192u

// The three keys, in the order they are written.
static const char *const KEYS[] = { "scene_wide", "inspector_wide",
				    "topbar_high" };
#define KEY_COUNT (sizeof KEYS / sizeof KEYS[0])

// The field KEYS[key] names.
static float *field(voe_editor_settings *settings, size_t key)
{
	return key == 0 ? &settings->scene_wide :
	       key == 1 ? &settings->inspector_wide :
			  &settings->topbar_high;
}

// Which of KEYS line starts with as a whole word, or KEY_COUNT for none.
static size_t key_of(const char *line)
{
	size_t length = strcspn(line, " \t");
	size_t key;

	for (key = 0; key < KEY_COUNT; key++)
		if (strlen(KEYS[key]) == length &&
		    strncmp(line, KEYS[key], length) == 0)
			return key;
	return KEY_COUNT;
}

// The file's folder, pushed into scratch; NULL when there is no settings
// folder.
static const char *settings_dir(voe_base_arena *scratch)
{
	const char *settings = voe_platform_folder_settings(scratch);

	if (settings == NULL)
		return NULL;
	return voe_platform_path_join(scratch, settings, SETTINGS_FOLDER);
}

// The whole file in dir, NUL-terminated, pushed into scratch. NULL when dir
// is NULL or there is no file.
static char *file_text(voe_base_arena *scratch, const char *dir)
{
	const char *path;
	size_t size;

	if (dir == NULL)
		return NULL;
	path = voe_platform_path_join(scratch, dir, SETTINGS_FILE);
	if (!voe_platform_file_exists(path))
		return NULL;
	return (char *)voe_platform_file_read(path, scratch, &size, NULL);
}

void voe_editor_settings_read(voe_editor_settings *settings)
{
	voe_base_arena *scratch;
	const char *dir;
	char *line;
	char *next;
	char *rest;
	size_t key;
	float value;

	VOE_BASE_ASSERT(settings != NULL, "reading settings into nothing");

	scratch = voe_base_arena_new(SETTINGS_SCRATCH);
	dir = settings_dir(scratch);
	for (line = file_text(scratch, dir); line != NULL && *line != '\0';
	     line = next) {
		next = line + strcspn(line, "\n");
		if (*next == '\n')
			*next++ = '\0';
		key = key_of(line);
		if (key == KEY_COUNT)
			continue;
		value = strtof(line + strlen(KEYS[key]), &rest);
		if (rest == line + strlen(KEYS[key]) ||
		    rest[strspn(rest, " \t\r")] != '\0')
			continue;
		// Written the long way round so a NaN, which no comparison is
		// true of, is out of range like any other bad number.
		if (!(value <= SETTINGS_MOST) ||
		    !(key == 2 ? value >= 0.0f : value > 0.0f))
			continue;
		*field(settings, key) = value;
	}
	voe_base_arena_destroy(scratch);
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

bool voe_editor_settings_write(const voe_editor_settings *settings)
{
	voe_base_arena *scratch;
	voe_editor_settings copy;
	const char *dir;
	char *old;
	char *line;
	char *next;
	char *text;
	size_t capacity;
	size_t used = 0;
	size_t length;
	size_t key;
	bool ok = false;

	VOE_BASE_ASSERT(settings != NULL, "writing no settings");

	copy = *settings;
	scratch = voe_base_arena_new(SETTINGS_SCRATCH);
	dir = settings_dir(scratch);
	old = file_text(scratch, dir);
	// `<settings>` itself may be missing on a first write ever.
	if (dir != NULL &&
	    ensure_folder(voe_platform_folder_settings(scratch), scratch) &&
	    ensure_folder(dir, scratch)) {
		capacity = (old != NULL ? strlen(old) + 1 : 0) +
			   SETTINGS_OWN_ROOM;
		text = voe_base_arena_push(scratch, capacity);
		for (line = old; line != NULL && *line != '\0'; line = next) {
			length = strcspn(line, "\n");
			next = line[length] == '\n' ? line + length + 1 :
						      line + length;
			if (length == 0 || key_of(line) != KEY_COUNT)
				continue;
			memcpy(text + used, line, length);
			used += length;
			text[used++] = '\n';
		}
		for (key = 0; key < KEY_COUNT; key++)
			used += (size_t)snprintf(text + used, capacity - used,
						 "%s %.3f\n", KEYS[key],
						 (double)*field(&copy, key));
		ok = voe_platform_file_write(
			voe_platform_path_join(scratch, dir, SETTINGS_FILE),
			(const uint8_t *)text, used, NULL);
	}
	voe_base_arena_destroy(scratch);

	if (!ok)
		VOE_BASE_ERROR("editor",
			       "the panel sizes could not be written to "
			       "<settings>/%s/%s",
			       SETTINGS_FOLDER, SETTINGS_FILE);
	return ok;
}
