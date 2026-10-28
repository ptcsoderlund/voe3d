// `<settings>/voe3d/editor_settings`: read line by line as a key and a number
// or a `0`/`1` flag, and written back with every other key's line kept, by
// making the two folders above it as needed. The panel and snap halves, and
// settings_layout.c, share one line read (voe_editor_settings_lines_read) and
// one keep-other-keys write (voe_editor_settings_lines_write). See the header
// for the shape of the contract.
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

// Room for one line this file writes: a key and any finite double `%.3f`
// prints, which is under 330 characters.
#define SETTINGS_LINE_ROOM 352u

// Every key this file owns, in the order they are written: the panel's sizes,
// from KEY_FIRST_FLAG its open flags, and from KEY_SNAP the snap toggle and
// its three steps.
static const char *const KEYS[] = { "scene_wide",     "inspector_wide",
				    "assets_tall",    "topbar_high",
				    "view_share",     "scene_open",
				    "assets_open",    "inspector_open",
				    "view_open",      "snap",
				    "snap_move",      "snap_rotate",
				    "snap_scale" };
#define KEY_COUNT (sizeof KEYS / sizeof KEYS[0])
#define KEY_TOPBAR 3u
#define KEY_SHARE 4u
#define KEY_FIRST_FLAG 5u
#define KEY_SNAP 9u
#define KEY_SNAP_MOVE 10u
#define KEY_SNAP_ROTATE 11u

// The open flag KEYS[key] names; key is a panel flag's.
static bool *flag(voe_editor_settings *settings, size_t key)
{
	VOE_BASE_ASSERT(key >= KEY_FIRST_FLAG && key < KEY_SNAP,
			"no open flag for that key");
	return key == 5 ? &settings->scene_open :
	       key == 6 ? &settings->assets_open :
	       key == 7 ? &settings->inspector_open :
			  &settings->view_open;
}

// The step KEYS[key] names; key is after KEY_SNAP.
static float *step(voe_editor_snap *snap, size_t key)
{
	VOE_BASE_ASSERT(key > KEY_SNAP && key < KEY_COUNT,
			"no snap step for that key");
	return key == KEY_SNAP_MOVE   ? &snap->move :
	       key == KEY_SNAP_ROTATE ? &snap->rotate :
					&snap->scale;
}

// The flag a line's text after its key gives, as 0 or 1, or -1 when that text
// is not exactly one `0` or `1` between blanks.
static int flag_value(const char *rest)
{
	rest += strspn(rest, " \t");
	if ((*rest != '0' && *rest != '1') ||
	    rest[1 + strspn(rest + 1, " \t\r")] != '\0')
		return -1;
	return *rest - '0';
}

// Whether a line's text after its key is one number between blanks, put in
// value when it is.
static bool number(const char *rest, double *value)
{
	char *end;

	VOE_BASE_ASSERT(rest != NULL && value != NULL, "no number to read");
	*value = strtod(rest, &end);
	return end != rest && end[strspn(end, " \t\r")] == '\0';
}

// The size field KEYS[key] names; key is not KEY_SHARE.
static float *field(voe_editor_settings *settings, size_t key)
{
	VOE_BASE_ASSERT(key < KEY_SHARE, "no size field for that key");
	return key == 0 ? &settings->scene_wide :
	       key == 1 ? &settings->inspector_wide :
	       key == 2 ? &settings->assets_tall :
			  &settings->topbar_high;
}

// Whether value may be put in the field KEYS[key] names. Written the long way
// round so a NaN, which no comparison is true of, is out of range like any
// other bad number; the share's and the steps' bounds keep out the infinities
// too.
static bool in_range(size_t key, double value)
{
	if (key == KEY_SHARE)
		return value > 0.0 && value < 1.0;
	if (key == KEY_SNAP_MOVE)
		return value >= VOE_EDITOR_SNAP_MOVE_LEAST &&
		       value <= VOE_EDITOR_SNAP_MOVE_MOST;
	if (key == KEY_SNAP_ROTATE)
		return value >= VOE_EDITOR_SNAP_ROTATE_LEAST &&
		       value <= VOE_EDITOR_SNAP_ROTATE_MOST;
	if (key > KEY_SNAP)
		return value >= VOE_EDITOR_SNAP_SCALE_LEAST &&
		       value <= VOE_EDITOR_SNAP_SCALE_MOST;
	return value <= (double)SETTINGS_MOST &&
	       (key == KEY_TOPBAR ? value >= 0.0 : value > 0.0);
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

void voe_editor_settings_lines_read(void (*take)(void *into, const char *line),
				    void *into)
{
	voe_base_arena *scratch;
	char *line;
	char *next;

	VOE_BASE_ASSERT(take != NULL, "reading lines into no taker");
	VOE_BASE_ASSERT(into != NULL, "reading lines into nothing");

	scratch = voe_base_arena_new(SETTINGS_SCRATCH);
	for (line = file_text(scratch, settings_dir(scratch));
	     line != NULL && *line != '\0'; line = next) {
		next = line + strcspn(line, "\n");
		if (*next == '\n')
			*next++ = '\0';
		take(into, line);
	}
	voe_base_arena_destroy(scratch);
}

// One panel line into the voe_editor_settings at into; snap lines and other
// keys' are not its.
static void panel_take(void *into, const char *line)
{
	voe_editor_settings *settings = into;
	size_t key = key_of(line);
	const char *rest;
	double value;
	int open;

	if (key >= KEY_SNAP)
		return;
	rest = line + strlen(KEYS[key]);
	if (key >= KEY_FIRST_FLAG) {
		open = flag_value(rest);
		if (open >= 0)
			*flag(settings, key) = open == 1;
		return;
	}
	if (!number(rest, &value) || !in_range(key, value))
		return;
	if (key == KEY_SHARE)
		settings->view_share = value;
	else
		*field(settings, key) = (float)value;
}

void voe_editor_settings_read(voe_editor_settings *settings)
{
	VOE_BASE_ASSERT(settings != NULL, "reading settings into nothing");
	voe_editor_settings_lines_read(panel_take, settings);
}

// One snap line into the voe_editor_snap at into; panel lines and other keys'
// are not its.
static void snap_take(void *into, const char *line)
{
	voe_editor_snap *snap = into;
	size_t key = key_of(line);
	const char *rest;
	double value;
	int on;

	if (key < KEY_SNAP || key == KEY_COUNT)
		return;
	rest = line + strlen(KEYS[key]);
	if (key == KEY_SNAP) {
		on = flag_value(rest);
		if (on >= 0)
			snap->on = on == 1;
		return;
	}
	if (number(rest, &value) && in_range(key, value))
		*step(snap, key) = (float)value;
}

voe_editor_snap voe_editor_settings_snap_read(void)
{
	voe_editor_snap snap = { .on = false,
				 .move = VOE_EDITOR_SNAP_MOVE,
				 .rotate = VOE_EDITOR_SNAP_ROTATE,
				 .scale = VOE_EDITOR_SNAP_SCALE };

	voe_editor_settings_lines_read(snap_take, &snap);
	VOE_BASE_ASSERT(snap.move > 0.0f && snap.rotate > 0.0f &&
				snap.scale > 0.0f,
			"a snap step read out of range");
	return snap;
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

bool voe_editor_settings_lines_write(bool (*own_line)(const char *line),
				     const char *own, size_t used)
{
	voe_base_arena *scratch;
	const char *dir;
	char *old;
	char *line;
	char *next;
	char *text;
	size_t kept = 0;
	size_t length;
	bool ok = false;

	VOE_BASE_ASSERT(own_line != NULL, "writing lines with no test of whose");
	VOE_BASE_ASSERT(own != NULL, "writing no lines");

	scratch = voe_base_arena_new(SETTINGS_SCRATCH);
	dir = settings_dir(scratch);
	old = file_text(scratch, dir);
	// `<settings>` itself may be missing on a first write ever.
	if (dir != NULL &&
	    ensure_folder(voe_platform_folder_settings(scratch), scratch) &&
	    ensure_folder(dir, scratch)) {
		text = voe_base_arena_push(
			scratch, (old != NULL ? strlen(old) + 1 : 0) + used);
		for (line = old; line != NULL && *line != '\0'; line = next) {
			length = strcspn(line, "\n");
			next = line[length] == '\n' ? line + length + 1 :
						      line + length;
			// Cut, so the test reads this line and not the next.
			line[length] = '\0';
			if (length == 0 || own_line(line))
				continue;
			memcpy(text + kept, line, length);
			kept += length;
			text[kept++] = '\n';
		}
		memcpy(text + kept, own, used);
		ok = voe_platform_file_write(
			voe_platform_path_join(scratch, dir, SETTINGS_FILE),
			(const uint8_t *)text, kept + used, NULL);
	}
	voe_base_arena_destroy(scratch);
	return ok;
}

// Whether a line is one of the panel keys', which the panel write replaces.
static bool panel_line(const char *line)
{
	return key_of(line) < KEY_SNAP;
}

// Whether a line is one of the snap keys', which the snap write replaces.
static bool snap_line(const char *line)
{
	const size_t key = key_of(line);

	return key >= KEY_SNAP && key < KEY_COUNT;
}

bool voe_editor_settings_write(const voe_editor_settings *settings)
{
	char own[KEY_SNAP * SETTINGS_LINE_ROOM];
	voe_editor_settings copy;
	size_t used = 0;
	size_t key;
	bool ok;

	VOE_BASE_ASSERT(settings != NULL, "writing no settings");

	copy = *settings;
	for (key = 0; key < KEY_FIRST_FLAG; key++)
		used += (size_t)snprintf(own + used, sizeof own - used,
					 "%s %.3f\n", KEYS[key],
					 key == KEY_SHARE ?
						 copy.view_share :
						 (double)*field(&copy, key));
	for (; key < KEY_SNAP; key++)
		used += (size_t)snprintf(own + used, sizeof own - used,
					 "%s %d\n", KEYS[key],
					 *flag(&copy, key) ? 1 : 0);
	ok = voe_editor_settings_lines_write(panel_line, own, used);

	if (!ok)
		VOE_BASE_ERROR("editor",
			       "the panel layout could not be written to "
			       "<settings>/%s/%s",
			       SETTINGS_FOLDER, SETTINGS_FILE);
	return ok;
}

bool voe_editor_settings_snap_write(const voe_editor_snap *snap)
{
	char own[(KEY_COUNT - KEY_SNAP) * SETTINGS_LINE_ROOM];
	voe_editor_snap copy;
	size_t used;
	size_t key;
	bool ok;

	VOE_BASE_ASSERT(snap != NULL, "writing no snap");

	copy = *snap;
	used = (size_t)snprintf(own, sizeof own, "%s %d\n", KEYS[KEY_SNAP],
				copy.on ? 1 : 0);
	for (key = KEY_SNAP + 1; key < KEY_COUNT; key++)
		used += (size_t)snprintf(own + used, sizeof own - used,
					 "%s %.3f\n", KEYS[key],
					 (double)*step(&copy, key));
	ok = voe_editor_settings_lines_write(snap_line, own, used);

	if (!ok)
		VOE_BASE_ERROR("editor",
			       "the snap settings could not be written to "
			       "<settings>/%s/%s",
			       SETTINGS_FOLDER, SETTINGS_FILE);
	return ok;
}
