# 0413 — The calls Windows deprecates are replaced, and the environment is read through platform
date: 2026-10-13
by: planner

## Decision
For 087 bug 05, carrying out ADR-0159 for every call the MSVC headers deprecate in code that builds on Windows.
No Linux guard (the sponsor, 2026-10-13).

1. **The environment is platform's.** `include/platform/environment.h` declares
   `const char *voe_platform_environment_read(const char *name, voe_base_arena *arena)`: the variable's value
   as UTF-8 on the arena, NULL when it is not set. Linux reads `getenv` in `src/environment_wayland.c`; Windows
   reads `GetEnvironmentVariableW` in `src/environment_win32.c` through `wide_win32.h`, which takes over
   `folder_win32.c`'s private reader. `render/src/instance.c` reads `VOE_RENDER_BEST_PRACTICES` (0398) with it.
2. **A copy in `src/` is a `memcpy`** of the measured length and its NUL, after the bound check already there.
3. **A copy in a test is `snprintf(field, sizeof field, "%s", text)`**: one shape for every test, bounded by
   the field, and not deprecated.
4. **`app/tests/picture.c` writes into the working directory**, as `authoring/tests/scene_cook.c` does, and
   reads no environment.
5. **`authoring/tests/scene_cook.c` keeps `fopen`**, suppressed at the one call as 0159 says: it writes files
   for `clang` to compile, `authoring` does not link `platform`, and the C library is the only writer it has.

## Reasoning
`getenv` cannot be written ourselves outside `platform`: the environment is the operating system's, and on
Windows it must be read wide (0248) to be UTF-8 (0247). One reader serves render and `folder_win32.c` both.
`memcpy` and `snprintf` are base C on both platforms, so no `#ifdef` splits a replacement. A test's temp folder
needed the environment only to pick a place; the working directory is the build tree's and already used.
Alternatives: a `testing/` file writer for `scene_cook.c` (0159 deferred it; one caller writing for a compiler
is not the shape the other three suppressions have); `strncpy` in tests (deprecated as well).

## Replaces
Nothing. Applies ADR-0159; amends 0398 only in how the variable is read.
