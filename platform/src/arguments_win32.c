// The Windows half of platform/arguments.h: GetCommandLineW split by
// CommandLineToArgvW, each entry made UTF-8 into the arena through
// platform/src/wide_win32.h, and the split freed with LocalFree (ADR-0248).
// argv is ignored, since it is in the legacy code page, except as the fallback
// when CommandLineToArgvW fails. Links shell32, which cmake/voe.cmake adds.
//
// RULE 6 DEVIATION, the one platform/arguments.h names: the list built here is
// argv's own shape, an array of strings, so it is a pointer to pointers.
// It is spelt as a pointer to the array of arguments, and no type alias stands
// in for a string.
#include <platform/arguments.h>

#include "wide_win32.h"

#include <base/assert.h>

#include <windows.h>
#include <shellapi.h>

voe_platform_arguments voe_platform_arguments_read(int argc, char *argv[],
						   voe_base_arena *arena)
{
	LPWSTR *wide;
	const char *(*values)[];
	int count;

	VOE_BASE_DEBUG_ASSERT(argc >= 0 && argv != NULL, "reading arguments main did not give");
	VOE_BASE_DEBUG_ASSERT(arena != NULL, "reading arguments into no arena");

	wide = CommandLineToArgvW(GetCommandLineW(), &count);
	if (wide == NULL)
		return (voe_platform_arguments){.count = argc,
						.values = (const char *const *)argv};

	values = voe_base_arena_push(arena, ((size_t)count + 1) * sizeof((*values)[0]));
	for (int i = 0; i < count; i++)
		(*values)[i] = voe_platform_utf8_from_wide(wide[i], arena);
	(*values)[count] = NULL;
	LocalFree(wide);
	return (voe_platform_arguments){.count = count, .values = *values};
}
