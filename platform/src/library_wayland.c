// The Linux half of platform/library.h: dlopen and dlsym, and nothing else.
//
// THE FILE NAME SAYS wayland AND THIS FILE HAS NOTHING TO DO WITH WAYLAND. The
// build drops a source whose name ends in _wayland on Windows and one ending in
// _win32 on Linux, and those two suffixes are the whole of its vocabulary for
// "one platform only" — see cmake/voe.cmake. Linux here means Linux; the window
// system does not come into it. Renaming the suffixes to platform words rather
// than window-system words is a change to voe.cmake and is written up on card
// 007, not made here.
//
// dlopen's handle is handed back as the opaque voe_platform_library * rather
// than wrapped in a struct of our own. There is nothing to add to it, and a
// wrapper would be an allocation that can fail on a path whose whole point is
// that the failure it reports is "not installed".
//
// RTLD_NOW resolves everything on open, so a library missing a symbol we will
// need fails here rather than the first time it is called. RTLD_LOCAL keeps its
// symbols out of the global namespace, which matters for a graphics loader that
// may itself dlopen a driver.
#include <platform/library.h>

#include <base/assert.h>

#include <dlfcn.h>

#include <string.h>

voe_platform_library *voe_platform_library_new(const char *name)
{
	VOE_BASE_DEBUG_ASSERT(name != NULL, "opening a library with no name");

	return dlopen(name, RTLD_NOW | RTLD_LOCAL);
}

void voe_platform_library_destroy(voe_platform_library *library)
{
	VOE_BASE_DEBUG_ASSERT(library != NULL, "closing a NULL library");

	dlclose(library);
}

voe_platform_symbol voe_platform_library_symbol(voe_platform_library *library,
						const char *name)
{
	void *found;
	voe_platform_symbol symbol;

	VOE_BASE_DEBUG_ASSERT(library != NULL, "asking a NULL library");
	VOE_BASE_DEBUG_ASSERT(name != NULL, "asking for a symbol with no name");

	found = dlsym(library, name);
	if (found == NULL)
		return NULL;

	// dlsym returns an object pointer for what is a function, which ISO C
	// does not let us cast; POSIX requires the two to be the same size and
	// says this is how you do it. -Wpedantic rejects the cast and says
	// nothing about the copy, which is the right way round: the copy is the
	// one that is defined.
	memcpy(&symbol, &found, sizeof(symbol));
	return symbol;
}
