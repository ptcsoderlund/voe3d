// The Windows half of platform/folder.h: FindFirstFileA/FindNextFileA,
// CreateDirectoryA, and GetEnvironmentVariableA for the home and settings
// folders. Read the header first — what each call promises and which failure
// is which is written there.
//
// FindFirstFileA AND NOT FindFirstFileW, for the same reason CreateFileA is
// used in platform/src/file_win32.c: a path here is a name the engine or its
// caller spelled in ASCII.
//
// TWO PASSES OVER THE SAME FOLDER, FOR THE SAME REASON platform/src/
// folder_wayland.c makes two: the arena's array has to be exactly the right
// size before anything is pushed into it (base/arena.h). FindFirstFileA has no
// rewind, so the second pass opens a fresh search rather than reusing the
// first one's handle.
//
// SORTING IS BY HAND, NOT qsort — see platform/src/folder_wayland.c's header
// for why, which applies here unchanged.
//
// A DIRECTORY SYMLINK ALREADY CARRIES FILE_ATTRIBUTE_DIRECTORY. Windows sets
// it on the reparse point itself for one made with the "directory" kind of
// symbolic link, so folder below needs no extra call to answer "is this a
// folder" the way the Linux side needs fstatat — there is nothing this file
// has to follow.
//
// NO getenv (ADR-0159): the environment is read with GetEnvironmentVariableA,
// which is also how a value's length is found before it is pushed into an
// arena sized for it.
#include <platform/folder.h>

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

#include <stdio.h>
#include <string.h>

// Long enough for any path this engine constructs plus "\*"; a path this long
// is a call-site mistake, not a runtime condition — see
// platform/src/file_win32.c's PARTIAL_PATH_MAX for the same trade.
#define PATTERN_PATH_MAX 4096

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

bool voe_platform_folder_list(const char *path, voe_base_arena *arena,
			      voe_platform_folder_listing *out,
			      voe_base_error *error)
{
	char pattern[PATTERN_PATH_MAX];
	WIN32_FIND_DATAA data;
	HANDLE search;
	uint32_t count = 0;
	voe_platform_folder_entry *entries = NULL;
	uint32_t filled = 0;

	VOE_BASE_ASSERT(path != NULL, "listing a folder with no path");
	VOE_BASE_ASSERT(arena != NULL, "listing a folder into no arena");
	VOE_BASE_ASSERT(out != NULL, "listing a folder with nowhere to put it");
	VOE_BASE_ASSERT(strlen(path) + strlen("\\*") < sizeof pattern,
			"path is too long for voe_platform_folder_list");
	(void)snprintf(pattern, sizeof pattern, "%s\\*", path);

	search = FindFirstFileA(pattern, &data);
	if (search == INVALID_HANDLE_VALUE) {
		VOE_BASE_ERROR("platform", "could not open %s: error %lu", path,
			       (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}
	do {
		if (!is_dot_entry(data.cFileName))
			count++;
	} while (FindNextFileA(search, &data));
	(void)FindClose(search);

	// A push of zero bytes has no caller (base/arena.h) — an empty folder
	// leaves entries NULL and out->count 0, which the header promises is a
	// valid, indexable-nothing listing.
	if (count > 0)
		entries = voe_base_arena_push(arena, count * sizeof *entries);

	search = FindFirstFileA(pattern, &data);
	if (search == INVALID_HANDLE_VALUE) {
		VOE_BASE_ERROR("platform",
			       "could not reopen %s while listing it: error %lu",
			       path, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}
	do {
		size_t length;
		char *name;

		if (is_dot_entry(data.cFileName))
			continue;
		if (filled >= count)
			break;

		length = strlen(data.cFileName);
		name = voe_base_arena_push(arena, length + 1);
		memcpy(name, data.cFileName, length);

		entries[filled].name = name;
		entries[filled].folder =
			(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		// ADR-0166: hidden is one meaning on every platform — a name
		// beginning with '.', which is this engine's own convention
		// and not Windows', so it is checked here regardless of what
		// FindFirstFileA reports — with FILE_ATTRIBUTE_HIDDEN marking
		// a further entry hidden on top of it.
		entries[filled].hidden =
			data.cFileName[0] == '.' ||
			(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
		filled++;
	} while (FindNextFileA(search, &data));
	(void)FindClose(search);

	// Insertion sort, ascending by byte order of name — see
	// platform/src/folder_wayland.c's header for why this is not qsort.
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

	if (CreateDirectoryA(path, NULL)) {
		report(error, VOE_BASE_OK);
		return true;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		VOE_BASE_ERROR("platform", "%s already exists", path);
		report(error, VOE_BASE_ERROR_REFUSED);
		return false;
	}

	VOE_BASE_ERROR("platform", "could not create %s: error %lu", path,
		       (unsigned long)GetLastError());
	report(error, VOE_BASE_ERROR_UNAVAILABLE);
	return false;
}

// Reads name out of the environment into arena, dropping one trailing '\\' or
// '/' if there is one — every path this file hands back promises none — and
// answers NULL for a variable the environment never set or set to nothing.
static const char *read_variable(voe_base_arena *arena, const char *name)
{
	DWORD needed;
	char *buffer;
	size_t length;

	needed = GetEnvironmentVariableA(name, NULL, 0);
	if (needed <= 1)
		return NULL;

	buffer = voe_base_arena_push(arena, needed);
	(void)GetEnvironmentVariableA(name, buffer, needed);

	length = strlen(buffer);
	if (length > 1 && (buffer[length - 1] == '\\' || buffer[length - 1] == '/'))
		buffer[length - 1] = '\0';
	return buffer;
}

const char *voe_platform_folder_home(voe_base_arena *arena)
{
	VOE_BASE_ASSERT(arena != NULL, "finding the home folder into no arena");

	return read_variable(arena, "USERPROFILE");
}

const char *voe_platform_folder_settings(voe_base_arena *arena)
{
	VOE_BASE_ASSERT(arena != NULL,
			"finding the settings folder into no arena");

	return read_variable(arena, "APPDATA");
}
