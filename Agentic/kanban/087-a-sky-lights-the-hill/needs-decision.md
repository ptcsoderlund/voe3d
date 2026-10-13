# Needs decision — bug 05: may Linux be made to refuse the calls Windows deprecates?

## Question
Bug 05's Expected asks that the fix "keeps them from coming back on Linux, where nothing warns". ADR-0159
(in force, never replaced) says the opposite: each such call is found by building on Windows, there is no
list and nothing greps for the class, and the Linux guard was declined by the sponsor on 2026-09-15. A guard
is a tree-wide build or check change, not this feature's to choose. Fixing the calls themselves is plannable
under 0159 now; whether a guard comes with it is the open part.

The calls in code that builds on Windows (the `_wayland.c` sources and the `#ifndef _WIN32` blocks of
`platform/tests/folder.c` and `platform/tests/trash.c` excluded):
- `src/`: `strcpy` in `assets/src/material.c:82` and `assets/src/landscape.c:206`; `getenv` in
  `render/src/instance.c:284`.
- `tests/`: `strcpy`/`strncpy` in `3d/tests` (draw_material, emitter_component, models_material,
  model_component), `assets/tests/material.c`, `audio/tests` (sound_component, sound_system),
  `game/tests` (models, steps), `scene/tests/prefab.c`; `getenv` in `app/tests/picture.c`; an unsuppressed
  `fopen` in `authoring/tests/scene_cook.c`. The `fopen` oracles in `app/tests/capture.c`,
  `game/tests/frame.c` and `platform/tests/file.c` already carry 0159's suppression.

## Options
1. **Fix every call, no guard (0159 as written).** `src/` calls replaced by our own code, test copies by
   `memcpy`/`snprintf` or a base string call, a test oracle `fopen` suppressed at the call per 0159. The next
   such call is found on Windows again, as 0159 accepts.
2. **Fix every call, and Linux warns as Windows does.** `cmake/voe.cmake` force-includes, on Linux only, a
   header that redeclares the names the MSVC CRT marks deprecated (`strcpy`, `strcat`, `strncpy`, `sprintf`,
   `sscanf`, `fopen`, `getenv`, `strtok`, the time converters, the POSIX names such as `strdup`) with
   `[[deprecated]]`. `-Werror` then fails on Linux at the same call with the same diagnostic, and 0159's
   per-call pragma for a test oracle works unchanged. The list is the CRT's, written once. Replaces 0159's
   "no list, nothing greps" and the sponsor's 2026-09-15 refusal.
3. **Fix every call, and `check.cmake` greps for the names.** 0159's rejected option: a second, weaker copy
   of what the compiler knows, blind to the per-call suppression unless it parses pragmas.

## Recommendation
Option 2 if the sponsor now wants the guard, which bug 05's Expected says: the compiler stays the authority,
the diagnostic and its suppression are the same on both platforms, and the cost is one header of
declarations. Otherwise option 1, and bug 05's Expected loses its last clause. Either way the fix is about
ten cards, one per folder (`assets`, `render`, and the `tests/` of `3d`, `assets`, `audio`, `game`,
`scene`, `app`, `authoring`), with option 2's `cmake` card last so the tree builds on Linux after it.
