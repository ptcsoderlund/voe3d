// The Windows half of platform/folder.h: FindFirstFileW/FindNextFileW,
// CreateDirectoryW, and GetEnvironmentVariableW for the home and settings
// folders. Read the header first — what each call promises and which failure
// is which is written there.
//
// A PATH IS UTF-8 AND IS CONVERTED ON THE WAY IN, AND A NAME ON THE WAY OUT
// (ADR-0247, ADR-0248), through platform/src/wide_win32.h. A path goes into a
// MAX_PATH wide stack buffer; one that does not fit fails as the call fails
// when the folder is not there. Each listed name and each environment value is
// made UTF-8 in the caller's arena.
//
// TWO PASSES OVER THE SAME FOLDER, FOR THE SAME REASON platform/src/
// folder_wayland.c makes two: the arena's array has to be exactly the right
// size before anything is pushed into it (base/arena.h). FindFirstFileW has no
// rewind, so the second pass opens a fresh search rather than reusing the
// first one's handle.
//
// SORTING IS BY HAND, NOT qsort — see platform/src/folder_wayland.c's header
// for why, which applies here unchanged. It is by the bytes of the UTF-8 name,
// as on Linux.
//
// A DIRECTORY SYMLINK ALREADY CARRIES FILE_ATTRIBUTE_DIRECTORY. Windows sets
// it on the reparse point itself for one made with the "directory" kind of
// symbolic link, so folder below needs no extra call to answer "is this a
// folder" the way the Linux side needs fstatat — there is nothing this file
// has to follow.
//
// NO getenv (ADR-0159): the environment is read with GetEnvironmentVariableW,
// which is also how a value's length is found before it is pushed into an
// arena sized for it.
#include <platform/folder.h>

#include "wide_win32.h"

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

#include <string.h>
#include <wchar.h>

// The out-parameter is optional (rule 13), so every path sets it through here
// rather than repeating the check twice.
static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

// True for a name this folder never lists: the entry itself and its parent.
static bool is_dot_entry(const wchar_t *name)
{
	return wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0;
}

bool voe_platform_folder_list(const char *path, voe_base_arena *arena,
			      voe_platform_folder_listing *out,
			      voe_base_error *error)
{
	static const wchar_t suffix[] = L"\\*";
	wchar_t pattern[MAX_PATH];
	WIN32_FIND_DATAW data;
	HANDLE search;
	uint32_t count = 0;
	voe_platform_folder_entry *entries = NULL;
	uint32_t filled = 0;

	VOE_BASE_ASSERT(path != NULL, "listing a folder with no path");
	VOE_BASE_ASSERT(arena != NULL, "listing a folder into no arena");
	VOE_BASE_ASSERT(out != NULL, "listing a folder with nowhere to put it");

	if (!voe_platform_wide_from_utf8(path, pattern, MAX_PATH) ||
	    wcslen(pattern) + wcslen(suffix) >= MAX_PATH) {
		VOE_BASE_ERROR("platform", "could not open %s: the path is too long",
			       path);
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}
	// memcpy, not wcscat, which the MSVC C runtime deprecates.
	(void)memcpy(pattern + wcslen(pattern), suffix, sizeof suffix);

	search = FindFirstFileW(pattern, &data);
	if (search == INVALID_HANDLE_VALUE) {
		VOE_BASE_ERROR("platform", "could not open %s: error %lu", path,
			       (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}
	do {
		if (!is_dot_entry(data.cFileName))
			count++;
	} while (FindNextFileW(search, &data));
	(void)FindClose(search);

	// A push of zero bytes has no caller (base/arena.h) — an empty folder
	// leaves entries NULL and out->count 0, which the header promises is a
	// valid, indexable-nothing listing.
	if (count > 0)
		entries = voe_base_arena_push(arena, count * sizeof *entries);

	search = FindFirstFileW(pattern, &data);
	if (search == INVALID_HANDLE_VALUE) {
		VOE_BASE_ERROR("platform",
			       "could not reopen %s while listing it: error %lu",
			       path, (unsigned long)GetLastError());
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}
	do {
		if (is_dot_entry(data.cFileName))
			continue;
		if (filled >= count)
			break;

		entries[filled].name =
			voe_platform_utf8_from_wide(data.cFileName, arena);
		entries[filled].folder =
			(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		// ADR-0166: hidden is one meaning on every platform — a name
		// beginning with '.', which is this engine's own convention
		// and not Windows', so it is checked here regardless of what
		// FindFirstFileW reports — with FILE_ATTRIBUTE_HIDDEN marking
		// a further entry hidden on top of it.
		entries[filled].hidden =
			data.cFileName[0] == L'.' ||
			(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
		filled++;
	} while (FindNextFileW(search, &data));
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
	wchar_t wide_path[MAX_PATH];

	VOE_BASE_ASSERT(path != NULL, "creating a folder with no path");

	if (!voe_platform_wide_from_utf8(path, wide_path, MAX_PATH)) {
		VOE_BASE_ERROR("platform", "could not create %s: the path is too long",
			       path);
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return false;
	}

	if (CreateDirectoryW(wide_path, NULL)) {
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

// Reads name out of the environment into arena as UTF-8, dropping one trailing
// '\\' or '/' if there is one — every path this file hands back promises none
// — and answers NULL for a variable the environment never set or set to
// nothing. The wide value is read into the arena too, since its length is only
// known at run time.
static const char *read_variable(voe_base_arena *arena, const wchar_t *name)
{
	DWORD needed;
	wchar_t *wide;
	char *buffer;
	size_t length;

	needed = GetEnvironmentVariableW(name, NULL, 0);
	if (needed <= 1)
		return NULL;

	wide = voe_base_arena_push(arena, needed * sizeof *wide);
	if (GetEnvironmentVariableW(name, wide, needed) == 0)
		return NULL;
	buffer = voe_platform_utf8_from_wide(wide, arena);

	length = strlen(buffer);
	if (length > 1 && (buffer[length - 1] == '\\' || buffer[length - 1] == '/'))
		buffer[length - 1] = '\0';
	return buffer;
}

const char *voe_platform_folder_home(voe_base_arena *arena)
{
	VOE_BASE_ASSERT(arena != NULL, "finding the home folder into no arena");

	return read_variable(arena, L"USERPROFILE");
}

const char *voe_platform_folder_settings(voe_base_arena *arena)
{
	VOE_BASE_ASSERT(arena != NULL,
			"finding the settings folder into no arena");

	return read_variable(arena, L"APPDATA");
}
