// The Windows half of platform/path.h: GetFullPathNameA to resolve a path
// against the working directory, and GetFileAttributesA to check something
// is actually there — GetFullPathNameA alone will happily hand back a path
// for a name that does not exist. Read the header first — what the call
// promises and which failure is which is written there.
//
// GetFullPathNameA AND NOT GetFullPathNameW, for the same reason
// CreateFileA is used in platform/src/file_win32.c: a path here is a name
// the engine or its caller spelled in ASCII.
//
// TWO CALLS TO SIZE THE BUFFER, THE SAME IDIOM platform/src/folder_win32.c's
// read_variable uses for GetEnvironmentVariableA: the first call with no
// buffer answers how large one needs to be, arena is pushed for exactly
// that, and the second call fills it.
#include <platform/path.h>

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

const char *voe_platform_path_absolute(const char *path, voe_base_arena *arena,
				       voe_base_error *error)
{
	DWORD needed;
	char *buffer;

	VOE_BASE_ASSERT(path != NULL, "resolving no path");
	VOE_BASE_ASSERT(arena != NULL, "resolving a path into no arena");

	needed = GetFullPathNameA(path, 0, NULL, NULL);
	VOE_BASE_ASSERT(needed > 0,
			"GetFullPathNameA could not size its own buffer");

	buffer = voe_base_arena_push(arena, needed);
	(void)GetFullPathNameA(path, needed, buffer, NULL);

	if (GetFileAttributesA(buffer) == INVALID_FILE_ATTRIBUTES) {
		VOE_BASE_ERROR("platform", "could not resolve %s: error %lu",
			       path, (unsigned long)GetLastError());
		if (error != NULL)
			*error = VOE_BASE_ERROR_UNAVAILABLE;
		return NULL;
	}

	if (error != NULL)
		*error = VOE_BASE_OK;
	return buffer;
}
