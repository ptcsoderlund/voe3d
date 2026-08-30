// The Windows half of platform/library.h: LoadLibraryA and GetProcAddress, and
// nothing else.
//
// The module handle is handed back as the opaque voe_platform_library * rather
// than wrapped in a struct of our own. There is nothing to add to it, and a
// wrapper would be an allocation that can fail on a path whose whole point is
// that the failure it reports is "not installed".
//
// LoadLibraryA and not LoadLibraryW, because a library name is an ASCII file
// name that this engine writes into its own source. A path from a user would be
// a different question and would arrive through a different function.
#include <platform/library.h>

#include <base/assert.h>

#include <windows.h>

voe_platform_library *voe_platform_library_new(const char *name)
{
	VOE_BASE_DEBUG_ASSERT(name != NULL, "opening a library with no name");

	return (voe_platform_library *)LoadLibraryA(name);
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
