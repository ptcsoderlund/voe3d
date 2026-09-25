// The Windows half of platform/library.h: LoadLibraryW and GetProcAddress, and
// nothing else.
//
// The module handle is handed back as the opaque voe_platform_library * rather
// than wrapped in a struct of our own. There is nothing to add to it, and a
// wrapper would be an allocation that can fail on a path whose whole point is
// that the failure it reports is "not installed".
//
// THE NAME IS UTF-8 AND CROSSES INTO UTF-16 THROUGH platform/src/wide_win32.h
// (ADR-0248), in a MAX_PATH wide stack buffer; a name that does not fit is NULL
// with the same report as a failed load.
//
// A failed load is reported with GetLastError's text from FormatMessageW, cut to
// a fixed buffer with its trailing line break dropped, and made UTF-8 into a
// buffer three bytes per unit wide, so a localised loader message reaches the
// report in UTF-8 and not the code page; no allocation.
#include <platform/library.h>

#include "wide_win32.h"

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

voe_platform_library *voe_platform_library_new(const char *name)
{
	wchar_t wide[MAX_PATH];
	wchar_t message[512];
	char reason[3 * 512];
	HMODULE library;
	DWORD error;
	DWORD length;

	VOE_BASE_DEBUG_ASSERT(name != NULL, "opening a library with no name");

	if (voe_platform_wide_from_utf8(name, wide, MAX_PATH)) {
		library = LoadLibraryW(wide);
		error = library == NULL ? GetLastError() : ERROR_SUCCESS;
	} else {
		library = NULL;
		error = ERROR_FILENAME_EXCED_RANGE;
	}
	if (library == NULL) {
		length = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, error, 0, message, 512, NULL);
		while (length > 0 && (message[length - 1] == L'\r' || message[length - 1] == L'\n'))
			length--;
		message[length] = L'\0';
		if (length == 0 || WideCharToMultiByte(CP_UTF8, 0, message, -1, reason,
						       (int)sizeof(reason), NULL, NULL) == 0)
			reason[0] = '\0';
		VOE_BASE_ERROR("platform", "cannot open library %s: %s (error %lu)", name,
			       reason[0] != '\0' ? reason : "no reason given",
			       (unsigned long)error);
	}
	return (voe_platform_library *)library;
}

void voe_platform_library_destroy(voe_platform_library *library)
{
	VOE_BASE_DEBUG_ASSERT(library != NULL, "closing a NULL library");

	FreeLibrary((HMODULE)library);
}

voe_platform_symbol voe_platform_library_symbol(voe_platform_library *library,
						const char *name)
{
	VOE_BASE_DEBUG_ASSERT(library != NULL, "asking a NULL library");
	VOE_BASE_DEBUG_ASSERT(name != NULL, "asking for a symbol with no name");

	// GetProcAddress already returns a function pointer, so unlike the
	// Linux side this is a cast between two function-pointer types, which C
	// defines as long as the result is called through the right signature.
	return (voe_platform_symbol)GetProcAddress((HMODULE)library, name);
}
