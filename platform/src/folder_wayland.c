// The Linux half of platform/folder.h: listing a folder, making one, and the
// home and settings folders. Read the header first — what each call promises
// and which failure is which is written there.
//
// LINUX AND NOT WAYLAND, DESPITE THE NAME — see platform/src/file_wayland.c's
// header for why a name ending in _wayland means "this platform" in the build
// and not "this file talks to a compositor". Nothing in here does.
//
// _POSIX_C_SOURCE 200809L is what makes <dirent.h> declare dirfd and
// <sys/stat.h> declare fstatat under -std=c23, the same feature-test macro
// platform/src/file_wayland.c already sets and for the same reason.
// _DEFAULT_SOURCE alongside it is what glibc's <dirent.h> needs to expose
// d_type and the DT_* names at all — they are not POSIX, and _POSIX_C_SOURCE
// alone hides them.
//
// TWO PASSES OVER THE SAME DIRECTORY, FOR THE SAME REASON authoring's scene
// writer makes two passes over its entities (authoring/src/scene_write.c): the
// arena's array has to be exactly the right size before anything is pushed
// into it, because two pushes are not guaranteed to sit next to each other
// (base/arena.h). The first pass counts; rewinddir puts the stream back at the
// start; the second pass fills, stopping at the counted size rather than
// overrunning the array if the folder somehow gained an entry between the two
// — nothing else touches a project's own folder while the editor has it open,
// so this is a safety margin and not a case this engine expects to hit.
//
// SORTING IS BY HAND, NOT qsort, for the reason authoring/src/authored.h
// gives: qsort's comparator is a function pointer, and this engine keeps those
// to render's loader table alone. A folder's own entry count is small enough
// that an insertion sort is the right cost for it.
//
// d_type IS TRUSTED WHEN IT IS NOT DT_UNKNOWN. Not every filesystem fills it
// in, so DT_UNKNOWN falls back to fstatat; DT_LNK does too, because the header
// promises a symlink to a folder counts as one, and fstatat follows the link
// by default.
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <platform/folder.h>

#include <base/assert.h>
#include <base/report.h>

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// The out-parameter is optional (rule 13), so every path sets it through here
// rather than repeating the check twice.
static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

// True for a name this folder never lists: the entry itself and its parent.
static bool is_dot_entry(const char *name)
{
	return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

// Whether name, inside the folder open as the descriptor fd, is itself a
// folder — following a symlink when d_type does not already say so. Takes the
// bare descriptor rather than dir itself so that dirfd() is called exactly
// once per listing (see below) and not once per unresolved entry.
static bool entry_is_folder(int fd, const char *name, unsigned char d_type)
{
	struct stat info;

	if (d_type == DT_DIR)
		return true;
	if (d_type != DT_UNKNOWN && d_type != DT_LNK)
		return false;
	if (fstatat(fd, name, &info, 0) != 0)
		return false;
	return S_ISDIR(info.st_mode);
}

bool voe_platform_folder_list(const char *path, voe_base_arena *arena,
			      voe_platform_folder_listing *out,
			      voe_base_error *error)
{
	DIR *dir;
	int fd;
	struct dirent *entry;
	uint32_t count = 0;
	voe_platform_folder_entry *entries = NULL;
	uint32_t filled = 0;

	VOE_BASE_ASSERT(path != NULL, "listing a folder with no path");
	VOE_BASE_ASSERT(arena != NULL, "listing a folder into no arena");
	VOE_BASE_ASSERT(out != NULL, "listing a folder with nowhere to put it");

	dir = opendir(path);
	if (dir == NULL) {
		VOE_BASE_ERROR("platform", "could not open %s: %s", path,
			       strerror(errno));
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}

	// Read once, right after a successful opendir: dirfd() can only fail on
	// a stream that is not truly open, which this one just proved it is, so
	// a failure here is this file's own bug and not the world's (rule 13).
	fd = dirfd(dir);
	VOE_BASE_ASSERT(fd >= 0, "an open directory stream with no descriptor");

	while ((entry = readdir(dir)) != NULL) {
		if (!is_dot_entry(entry->d_name))
			count++;
	}
	rewinddir(dir);

	// A push of zero bytes has no caller (base/arena.h) — an empty folder
	// leaves entries NULL and out->count 0, which the header promises is a
	// valid, indexable-nothing listing.
	if (count > 0)
		entries = voe_base_arena_push(arena, count * sizeof *entries);

	while (filled < count && (entry = readdir(dir)) != NULL) {
		size_t length;
		char *name;

		if (is_dot_entry(entry->d_name))
			continue;

		length = strlen(entry->d_name);
		name = voe_base_arena_push(arena, length + 1);
		memcpy(name, entry->d_name, length);

		entries[filled].name = name;
		entries[filled].folder = entry_is_folder(fd, entry->d_name,
							 entry->d_type);
		entries[filled].hidden = entry->d_name[0] == '.';
		filled++;
	}

	(void)closedir(dir);

	// Insertion sort, ascending by byte order of name — see the header
	// comment on why this is not qsort.
	for (uint32_t i = 1; i < filled; i++) {
		voe_platform_folder_entry key = entries[i];
		uint32_t j = i;

		while (j > 0 && strcmp(entries[j - 1].name, key.name) > 0) {
			entries[j] = entries[j - 1];
			j--;
		}
		entries[j] = key;
	}

	out->entries = entries;
	out->count = filled;
	report(error, VOE_BASE_OK);
	return true;
}

bool voe_platform_folder_create(const char *path, voe_base_error *error)
{
	VOE_BASE_ASSERT(path != NULL, "creating a folder with no path");

	if (mkdir(path, 0755) == 0) {
		report(error, VOE_BASE_OK);
		return true;
	}

	if (errno == EEXIST) {
		VOE_BASE_ERROR("platform", "%s already exists", path);
		report(error, VOE_BASE_ERROR_REFUSED);
		return false;
	}

	VOE_BASE_ERROR("platform", "could not create %s: %s", path,
		       strerror(errno));
	report(error, VOE_BASE_ERROR_UNAVAILABLE);
	return false;
}

// Copies text into arena, dropping one trailing '/' if there is one — every
// path this file hands back promises none — and answers NULL for a value the
// environment never set or set to nothing. Shared by _home and _settings'
// fallback.
static const char *copy_trimmed(voe_base_arena *arena, const char *text)
{
	size_t length;
	char *copy;

	if (text == NULL || text[0] == '\0')
		return NULL;

	length = strlen(text);
	if (length > 1 && text[length - 1] == '/')
		length--;

	copy = voe_base_arena_push(arena, length + 1);
	memcpy(copy, text, length);
	return copy;
}

const char *voe_platform_folder_home(voe_base_arena *arena)
{
	VOE_BASE_ASSERT(arena != NULL, "finding the home folder into no arena");

	return copy_trimmed(arena, getenv("HOME"));
}

const char *voe_platform_folder_settings(voe_base_arena *arena)
{
	const char *xdg = getenv("XDG_CONFIG_HOME");
	const char *home;
	size_t home_length;
	char *joined;

	VOE_BASE_ASSERT(arena != NULL,
			"finding the settings folder into no arena");

	// "set and absolute" — a relative XDG_CONFIG_HOME is not honoured, and
	// falls through to $HOME/.config with everyone else who left it unset.
	if (xdg != NULL && xdg[0] == '/')
		return copy_trimmed(arena, xdg);

	home = voe_platform_folder_home(arena);
	if (home == NULL)
		return NULL;

	home_length = strlen(home);
	joined = voe_base_arena_push(arena,
				     home_length + strlen("/.config") + 1);
	memcpy(joined, home, home_length);
	memcpy(joined + home_length, "/.config", strlen("/.config"));
	return joined;
}
