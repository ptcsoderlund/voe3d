# 0245 — On Windows the editor exports its engine through a `.def`, and the library links its import library
date: 2026-09-25
by: planner

## Decision
For 025 bug 01, the binding 0243 left to the planner. On the MSVC ABI (ADR-0026) a DLL's undefined
symbols do not bind to the program that loads it; they bind through an import library, and data
only through `__declspec(dllimport)`. So on WIN32:

1. **The editor exports every engine symbol by a generated module definition.** A build rule runs
   `cmake/exports.cmake` with `CMAKE_NM` over the archives of the editor's DEPENDS: every defined
   external `voe_` symbol, data marked `DATA`. In `game`'s archive a member that leaves
   `voe_game_project_register` or `voe_game_project_systems_run` undefined (`run.c`) is skipped,
   because those are the project's to define. The `.def` is a source of `voe_editor`, so the link
   writes the import library `voe_editor.lib` beside it. Linux keeps 0242's `ENABLE_EXPORTS`.
2. **Engine data a project may read is declared `VOE_BASE_IMPORTED`** (`base/imported.h`): empty,
   except `__declspec(dllimport)` on `_WIN32` when `VOE_BASE_IMPORTING` is defined, which
   `game.cmake` defines for `project` alone. Every `extern` object in a public header carries it.
   So a project takes the address of engine data at run time, never in a static initialiser.
3. **The library links the editor's import library**: the editor's `toolchain.h` names it
   (`VOE_TOOLCHAIN_EDITOR_IMPORTS`, empty off Windows), the LIBRARY configure passes it as
   `-DVOE_EDITOR_IMPORTS`, and `game.cmake` refuses library mode on WIN32 without it.
4. **The library exports its own symbols** (`WINDOWS_EXPORT_ALL_SYMBOLS`), so `voe_platform_symbol`
   finds `voe_game_project_register`; it is `Build/editor/project.dll`.

## Reasoning
Only exported and imported names cross an MSVC-ABI module boundary; the `.def` from the archives
exports all of the engine without a marker on every function. Data is the one place the compiler
must know, hence one macro on the few `extern` objects. Rejected: `__declspec` on every public
function (every header in every folder); linking the engine into the DLL (a second copy of every
global, input state included); MinGW-style auto-import (lld does it only for the MinGW runtime);
CMake's `WINDOWS_EXPORT_ALL_SYMBOLS` on the editor (it reads only the program's own objects, not
its static libraries).

## Replaces
Nothing; fills in 0243.
