// The UTF-8 / UTF-16 pair in platform/src/wide_win32.h. Read that header first:
// why the "W" calls, why flags 0, and why a path that does not fit is the
// caller's ordinary failure are written there.
//
// The UTF-8 copy is sized by asking WideCharToMultiByte first, so it is exact
// and needs no buffer of its own.
#include "wide_win32.h"

#include <base/assert.h>

#include <windows.h>

bool voe_platform_wide_from_utf8(const char *text, wchar_t *out, int capacity)
{
	VOE_BASE_ASSERT(text != NULL, "converting no text to UTF-16");
	VOE_BASE_ASSERT(out != NULL && capacity > 0,
			"converting text to UTF-16 into no buffer");

	return MultiByteToWideChar(CP_UTF8, 0, text, -1, out, capacity) != 0;
}

char *voe_platform_utf8_from_wide(const wchar_t *text, voe_base_arena *arena)
{
	int size;
	char *utf8;

	VOE_BASE_ASSERT(text != NULL, "converting no text to UTF-8");
	VOE_BASE_ASSERT(arena != NULL, "converting text to UTF-8 into no arena");

	// With flags 0 and a terminated input, only a bad argument makes this
	// 0; the terminator alone is 1.
	size = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
	VOE_BASE_ASSERT(size > 0, "UTF-16 text could not be measured as UTF-8");

	utf8 = voe_base_arena_push(arena, (size_t)size);
	(void)WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, size, NULL, NULL);
	return utf8;
}
