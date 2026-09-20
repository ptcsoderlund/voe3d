// `<settings>/voe3d/theme_scalars`: read line by line as two numbers and the
// name after them, written by making the two folders above it as needed. See
// the header for the shape of the contract.
#include "theme_scalars.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ui/theme.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The name inside <settings> this engine's own settings live under, and the
// file inside that holding one line per adjusted theme.
#define THEME_SCALARS_FOLDER "voe3d"
#define THEME_SCALARS_FILE "theme_scalars"

// Scratch for the two folders' paths, the file's path and the text written
// into it. A block size, not a limit.
#define THEME_SCALARS_SCRATCH (4u * 1024u)

// Room for one line's two numbers, the blanks between them and the newline,
// beside the identity's own bytes. Wide enough for any finite float `%.3f`
// prints, so composing a line can never be cut short.
#define THEME_SCALARS_NUMBERS 128u

// The two numbers and the rest of the line, or false for a line this file has
// nothing to say about. line is cut at its newline already and is written
// into: the identity points into it.
static bool parse_line(char *line, voe_editor_theme_scalars_line *out)
{
	char *rest;
	float contrast;
	float separation;

	contrast = strtof(line, &rest);
	if (rest == line)
		return false;
	line = rest;
	separation = strtof(line, &rest);
	if (rest == line)
		return false;

	// Written the long way round so that a NaN — which no comparison with
	// a bound is true of — is out of range like any other bad number.
	if (!(contrast >= VOE_UI_THEME_SCALAR_MIN &&
	      contrast <= VOE_UI_THEME_SCALAR_MAX) ||
	    !(separation >= VOE_UI_THEME_SCALAR_MIN &&
	      separation <= VOE_UI_THEME_SCALAR_MAX))
		return false;

	line = rest;
	while (*line == ' ' || *line == '\t')
		line++;
	if (*line == '\0')
		return false;

	out->identity = line;
	out->contrast_strength = contrast;
	out->surface_separation = separation;
	return true;
}

void voe_editor_theme_scalars_read(voe_editor_theme_scalars *scalars,
				   voe_base_arena *arena)
{
	const char *settings;
	const char *path;
	const uint8_t *bytes;
	size_t size;
	char *text;
	char *newline;

	VOE_BASE_ASSERT(scalars != NULL, "reading the slider values into nothing");

	*scalars = (voe_editor_theme_scalars){ 0 };

	settings = voe_platform_folder_settings(arena);
	if (settings == NULL)
		return;

	path = voe_platform_path_join(
		arena,
		voe_platform_path_join(arena, settings, THEME_SCALARS_FOLDER),
		THEME_SCALARS_FILE);
	if (!voe_platform_file_exists(path))
		return;

	bytes = voe_platform_file_read(path, arena, &size, NULL);
	if (bytes == NULL)
		return;

	// The bytes are NUL-terminated past the last one (platform/file.h), so
	// cutting each line at its newline leaves the last one terminated too.
	text = (char *)bytes;
	while (*text != '\0' &&
	       scalars->count < VOE_EDITOR_THEME_SCALARS_LINES) {
		newline = strchr(text, '\n');
		if (newline != NULL)
			*newline = '\0';
		if (parse_line(text, &scalars->lines[scalars->count]))
			scalars->count++;
		if (newline == NULL)
			break;
		text = newline + 1;
	}
}

const voe_editor_theme_scalars_line *
voe_editor_theme_scalars_find(const voe_editor_theme_scalars *scalars,
			      const char *identity)
{
	uint32_t i;

	VOE_BASE_ASSERT(scalars != NULL && identity != NULL,
			"looking up no theme's slider values");

	for (i = 0; i < scalars->count; i++)
		if (strcmp(scalars->lines[i].identity, identity) == 0)
			return &scalars->lines[i];
	return NULL;
}

void voe_editor_theme_scalars_set(voe_editor_theme_scalars *scalars,
				  voe_base_arena *arena, const char *identity,
				  float contrast, float separation)
{
	voe_editor_theme_scalars_line *line;
	size_t length;
	char *copy;
	uint32_t i;

	VOE_BASE_ASSERT(scalars != NULL && identity != NULL,
			"remembering no theme's slider values");

	line = NULL;
	for (i = 0; i < scalars->count; i++)
		if (strcmp(scalars->lines[i].identity, identity) == 0)
			line = &scalars->lines[i];

	if (line == NULL) {
		if (scalars->count == VOE_EDITOR_THEME_SCALARS_LINES)
			return;
		length = strlen(identity);
		copy = voe_base_arena_push(arena, length + 1);
		memcpy(copy, identity, length + 1);
		line = &scalars->lines[scalars->count++];
		line->identity = copy;
	}
	line->contrast_strength = contrast;
	line->surface_separation = separation;
}

void voe_editor_theme_scalars_forget(voe_editor_theme_scalars *scalars,
				     const char *identity)
{
	uint32_t i;

	VOE_BASE_ASSERT(scalars != NULL && identity != NULL,
			"forgetting no theme's slider values");

	for (i = 0; i < scalars->count; i++) {
		if (strcmp(scalars->lines[i].identity, identity) != 0)
			continue;
		memmove(&scalars->lines[i], &scalars->lines[i + 1],
			(scalars->count - i - 1) * sizeof(scalars->lines[0]));
		scalars->count--;
		return;
	}
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

bool voe_editor_theme_scalars_write(const voe_editor_theme_scalars *scalars)
{
	voe_base_arena *scratch;
	const char *settings;
	const char *dir;
	size_t capacity;
	size_t used;
	char *text;
	uint32_t i;
	bool ok = false;

	VOE_BASE_ASSERT(scalars != NULL, "writing no slider values");

	scratch = voe_base_arena_new(THEME_SCALARS_SCRATCH);
	settings = voe_platform_folder_settings(scratch);
	if (settings != NULL) {
		dir = voe_platform_path_join(scratch, settings,
					     THEME_SCALARS_FOLDER);
		if (ensure_folder(settings, scratch) &&
		    ensure_folder(dir, scratch)) {
			capacity = 1;
			for (i = 0; i < scalars->count; i++)
				capacity += strlen(scalars->lines[i].identity) +
					    THEME_SCALARS_NUMBERS;
			text = voe_base_arena_push(scratch, capacity);
			used = 0;
			for (i = 0; i < scalars->count; i++)
				used += (size_t)snprintf(
					text + used, capacity - used,
					"%.3f %.3f %s\n",
					(double)scalars->lines[i]
						.contrast_strength,
					(double)scalars->lines[i]
						.surface_separation,
					scalars->lines[i].identity);
			// Nothing remembered is still written, as the one
			// blank line a read skips: the file has to stop
			// holding what the last _forget dropped, and
			// platform/file.h refuses a write of no bytes at all.
			if (used == 0)
				text[used++] = '\n';
			ok = voe_platform_file_write(
				voe_platform_path_join(scratch, dir,
						       THEME_SCALARS_FILE),
				(const uint8_t *)text, used, NULL);
		}
	}
	voe_base_arena_destroy(scratch);

	if (!ok)
		VOE_BASE_ERROR("editor",
			       "the remembered slider values could not be "
			       "written to <settings>/%s/%s",
			       THEME_SCALARS_FOLDER, THEME_SCALARS_FILE);
	return ok;
}
