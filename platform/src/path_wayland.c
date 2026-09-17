// The Linux half of platform/path.h: resolving a path to an absolute one
// with realpath. Read the header first — what the call promises and which
// failure is which is written there.
//
// LINUX AND NOT WAYLAND, DESPITE THE NAME — see platform/src/file_wayland.c's
// header for why a name ending in _wayland means "this platform" in the
// build and not "this file talks to a compositor". Nothing in here does.
//
// _POSIX_C_SOURCE 200809L is what makes <stdlib.h> declare realpath under
// -std=c23, the same feature-test macro platform/src/folder_wayland.c
// already sets and for the same reason.
//
// PATH_MAX IS THE BUFFER realpath WRITES INTO, NOT A LIMIT THIS FOLDER
// INVENTS. It is the size Linux's own realpath(3) requires when it is not
// asked to allocate for itself, exactly as platform/src/file_win32.c's
// PARTIAL_PATH_MAX is a stack buffer sized for what one call needs and
// nothing this engine constructs is expected to exceed.
//
// _DEFAULT_SOURCE ALONGSIDE _POSIX_C_SOURCE is what glibc's <stdlib.h> needs
// to declare realpath under -std=c23, the same pairing
// platform/src/folder_wayland.c uses for <dirent.h>'s d_type.
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <platform/path.h>

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

const char *voe_platform_path_absolute(const char *path, voe_base_arena *arena,
				       voe_base_error *error)
{
	char resolved[PATH_MAX];
	size_t length;
	char *copy;

	VOE_BASE_ASSERT(path != NULL, "resolving no path");
	VOE_BASE_ASSERT(arena != NULL, "resolving a path into no arena");

	if (realpath(path, resolved) == NULL) {
		VOE_BASE_ERROR("platform", "could not resolve %s: %s", path,
			       strerror(errno));
		if (error != NULL)
			*error = VOE_BASE_ERROR_UNAVAILABLE;
		return NULL;
	}

	length = strlen(resolved);
	copy = voe_base_arena_push(arena, length + 1);
	memcpy(copy, resolved, length + 1);

	if (error != NULL)
		*error = VOE_BASE_OK;
	return copy;
}
