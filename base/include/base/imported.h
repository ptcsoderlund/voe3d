// The mark on engine data that a project's code library reads from another
// module.
//
// On the MSVC ABI a DLL reads another module's data only through
// `__declspec(dllimport)`: the compiler has to know, at the declaration, that
// the object lives behind an import table. Functions need no mark; they bind
// through the editor's import library without it (0245).
//
// It goes before the type of every `extern` object in a public header:
//
//     #include "base/imported.h"
//
//     extern VOE_BASE_IMPORTED const voe_base_samples voe_dev_frame_samples;
//
// `cmake/game.cmake` defines `VOE_BASE_IMPORTING` for the target `project` on
// Windows only, never for the engine, so the engine's own objects compile the
// mark to nothing and still define what they declare. Everywhere else, and off
// `_WIN32`, it is empty.
//
// AN IMPORTED ADDRESS IS NOT A CONSTANT. It is read from the import table when
// the library loads, so a project takes the address of engine data at run
// time, never in a static initialiser.
#pragma once

#if defined(_WIN32) && defined(VOE_BASE_IMPORTING)
#define VOE_BASE_IMPORTED __declspec(dllimport)
#else
#define VOE_BASE_IMPORTED
#endif
