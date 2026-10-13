// The Windows half of platform/environment.h: GetEnvironmentVariableW, the
// name made wide and the value made UTF-8 through platform/src/wide_win32.h
// (ADR-0247, ADR-0248). Read the header first.
//
// UNSET IS ERROR_ENVVAR_NOT_FOUND. A call that answers 0 is either a variable
// the environment does not have, which sets that error, or one set to nothing,
// which does not; the last error is cleared before each call so the two stay
// apart.
//
// MEASURED, THEN READ. The first call asks for the size with no buffer; the
// wide value is pushed onto the arena at that size, since it is only known at
// run time, and made UTF-8 there too. A value another thread grows between
// the two calls is measured again, READ_ATTEMPTS times at most; past that it
// is reported and answered NULL.
//
// A name that does not fit MAX_PATH wide units is answered NULL, as not set:
// no caller asks for one that long, and wide_win32.h's stack buffer is the
// shape every other call here takes.
#include <platform/environment.h>

#include "wide_win32.h"

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

#define READ_ATTEMPTS 3

const char *voe_platform_environment_read(const char *name,
					  voe_base_arena *arena)
{
	wchar_t wide_name[MAX_PATH];
	DWORD size;

	VOE_BASE_ASSERT(name != NULL, "reading an environment variable with no name");
	VOE_BASE_ASSERT(arena != NULL, "reading an environment variable into no arena");

	if (!voe_platform_wide_from_utf8(name, wide_name, MAX_PATH))
		return NULL;

	SetLastError(ERROR_SUCCESS);
	size = GetEnvironmentVariableW(wide_name, NULL, 0);
	if (size == 0) {
		if (GetLastError() == ERROR_ENVVAR_NOT_FOUND)
			return NULL;
		// Set to nothing: "" is one zeroed byte (base/arena.h).
		return voe_base_arena_push(arena, 1);
	}

	for (int attempt = 0; attempt < READ_ATTEMPTS; attempt++) {
		wchar_t *wide = voe_base_arena_push(arena, size * sizeof *wide);
		DWORD written;

		SetLastError(ERROR_SUCCESS);
		written = GetEnvironmentVariableW(wide_name, wide, size);
		if (written == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
			return NULL;
		// Less than size is the length without the terminator: it fit.
		if (written < size)
			return voe_platform_utf8_from_wide(wide, arena);
		size = written;
	}

	VOE_BASE_ERROR("platform", "could not read %s: it kept changing", name);
	return NULL;
}
