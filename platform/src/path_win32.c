// The Windows half of platform/path.h: GetFullPathNameW to resolve a path
// against the working directory, and GetFileAttributesW to check something
// is actually there — GetFullPathNameW alone will happily hand back a path
// for a name that does not exist. Read the header first — what the call
// promises and which failure is which is written there.
//
// THE PATH IS UTF-8 AND CROSSES INTO UTF-16 THROUGH platform/src/wide_win32.h
// (ADR-0248), so a name in any script resolves. The input goes through a
// MAX_PATH wide stack buffer; one that does not fit is NULL and UNAVAILABLE,
// as for a name that is not there. The resolved wide path is made UTF-8 into
// the caller's arena.
//
// TWO CALLS TO SIZE THE BUFFER: the first call with no buffer answers how
// large one needs to be, scratch in the caller's arena is pushed for exactly
// that, and the second call fills it. The wide scratch stays in the arena
// under the UTF-8 copy; it is small and the arena is the caller's.
#include <platform/path.h>

#include "wide_win32.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

// Reports path as unresolved and answers NULL.
static const char *unresolved(const char *path, DWORD reason,
			      voe_base_error *error)
{
	VOE_BASE_DEBUG_ASSERT(path != NULL, "reporting no path as unresolved");

	VOE_BASE_ERROR("platform", "could not resolve %s: error %lu", path,
		       (unsigned long)reason);
	if (error != NULL)
		*error = VOE_BASE_ERROR_UNAVAILABLE;
	return NULL;
}

const char *voe_platform_path_absolute(const char *path, voe_base_arena *arena,
				       voe_base_error *error)
{
	wchar_t wide[MAX_PATH];
	DWORD needed;
	wchar_t *buffer;

	VOE_BASE_ASSERT(path != NULL, "resolving no path");
	VOE_BASE_ASSERT(arena != NULL, "resolving a path into no arena");

	if (!voe_platform_wide_from_utf8(path, wide, MAX_PATH))
		return unresolved(path, ERROR_FILENAME_EXCED_RANGE, error);

	needed = GetFullPathNameW(wide, 0, NULL, NULL);
	VOE_BASE_ASSERT(needed > 0,
			"GetFullPathNameW could not size its own buffer");

	buffer = voe_base_arena_push(arena, (size_t)needed * sizeof(*buffer));
	(void)GetFullPathNameW(wide, needed, buffer, NULL);

	if (GetFileAttributesW(buffer) == INVALID_FILE_ATTRIBUTES)
		return unresolved(path, GetLastError(), error);

	if (error != NULL)
		*error = VOE_BASE_OK;
	return voe_platform_utf8_from_wide(buffer, arena);
}
