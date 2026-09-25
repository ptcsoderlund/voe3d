// A shared library, opened by name at run time, and a symbol looked up in it.
// The operating system's two calls — dlopen/dlsym, LoadLibrary/GetProcAddress —
// behind one that neither names.
//
//     voe_platform_library *loader = voe_platform_library_new("libvulkan.so.1");
//     if (loader == NULL)
//             return ...;                     // not installed; see below
//     voe_platform_symbol entry = voe_platform_library_symbol(loader, "vkGetInstanceProcAddr");
//     voe_platform_library_destroy(loader);   // every symbol from it dies here
//
// WHY THIS IS IN platform AND NOT IN THE FOLDER THAT WANTS IT. render opens the
// Vulkan loader by name because nothing in this engine links against Vulkan.
// Opening a library by name is dlfcn.h on Linux and windows.h on Windows, both
// of which are operating-system headers, and platform is the only folder allowed
// to include one. So the two calls live here, and the folder that wants a
// library passes a name and gets a pointer back.
//
// THE NAME IS THE CALLER'S, INCLUDING ITS PLATFORM-SPECIFIC SPELLING. This
// folder does not translate "vulkan" into libvulkan.so.1 or vulkan-1.dll and
// will not learn to: the name of a library is knowledge about that library, not
// about the operating system, and it belongs with whoever knows why it is being
// opened.
//
// FAILURE IS A RETURNED NULL, from both functions, because each has exactly one
// way to fail: a library that is not installed, and a symbol that is not in it.
// Neither is the program being wrong, so neither is an assert — a machine with
// no graphics driver is a machine we tell rather than abort on. A library that
// will not open is also reported through VOE_BASE_ERROR, naming it and the
// loader's own reason (a missing dependency, an unresolved symbol), which a
// caller may show from voe_base_report_error_first(); a missing symbol is not.
//
// A symbol is only valid while the library it came from is open. Destroying the
// library while a pointer out of it is still being called is a crash this code
// cannot detect and will not try to.
#pragma once

// Opaque, and it is the operating system's own handle rather than something
// allocated here — which is why there is nothing to run out of memory over and
// nothing to free but the library itself.
typedef struct voe_platform_library voe_platform_library;

// Any function pointer fits in this one and casts back to itself, which is the
// only guarantee C offers about function pointers and the reason this is not
// void *. The caller casts it to the signature it asked for.
typedef void (*voe_platform_symbol)(void);

// name is what the operating system knows the library by, with its extension.
// Returns NULL, reported with the loader's reason, if it is not installed or
// cannot be loaded.
[[nodiscard]] voe_platform_library *voe_platform_library_new(const char *name);
void voe_platform_library_destroy(voe_platform_library *library);

// Returns NULL if the library does not export that name.
[[nodiscard]] voe_platform_symbol
voe_platform_library_symbol(voe_platform_library *library, const char *name);
