// One environment variable, read by name, as UTF-8 (ADR-0247).
//
//     const char *value = voe_platform_environment_read("VOE_X", arena);
//     if (value == NULL)
//             ...;                        // not set
//
// THE ENVIRONMENT IS THIS FOLDER'S (ADR-0413): it is the operating system's,
// read by an OS call, and platform is the only folder allowed one. On Windows
// it must be read wide (ADR-0248) to come back UTF-8 at all, and getenv there
// is deprecated besides; one reader here serves every caller on both.
//
// UNSET AND EMPTY ARE TWO ANSWERS. NULL is a variable the environment does not
// have; "" is one it has with nothing in it. A caller that treats both alike
// says so itself.
//
// THE COPY IS THE CALLER'S TO KEEP. The value is copied onto arena with its
// terminator, so it stays as it was for as long as the arena does, whatever
// later changes the environment.
//
// A NULL name or arena is the caller's bug and aborts (rule 13).
#pragma once

#include <base/arena.h>

// name's value, copied onto arena as UTF-8, or NULL when it is not set.
const char *voe_platform_environment_read(const char *name,
					  voe_base_arena *arena);
