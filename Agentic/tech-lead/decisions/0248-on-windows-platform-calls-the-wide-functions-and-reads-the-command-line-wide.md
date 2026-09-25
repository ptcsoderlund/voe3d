# 0248 — On Windows `platform` calls the wide functions and reads the command line wide
date: 2026-09-25
by: planner

## Decision
For 0247 on Windows, every system call in `platform` that takes or returns a path, a name, an
environment value or a message is the "W" one, never the "A" one. There is no code-page manifest.

- One internal pair in `platform/src/wide_win32.h`: UTF-8 to UTF-16 into a caller's buffer, and
  UTF-16 to UTF-8 into an arena. Both call `MultiByteToWideChar` / `WideCharToMultiByte` with
  `CP_UTF8` and flags 0, so malformed UTF-8 and a lone surrogate become U+FFFD and never assert.
- A path handed to a call that takes no arena goes into a stack buffer of `MAX_PATH` wide units.
  A path that does not fit fails the way that call already fails when the open fails. The file
  calls without `\\?\` refuse longer paths anyway, so this sets no new limit.
- A name from the system (a listing entry, a resolved path, `%USERPROFILE%`, `%APPDATA%`, a loader
  message) becomes UTF-8 in the caller's arena, or in the scratch the call already uses.
- A program's own arguments come from `platform` as UTF-8. On Windows they come from
  `GetCommandLineW` and `CommandLineToArgvW`, so `platform` links `shell32` beside `user32` in
  `cmake/voe.cmake`. On Linux they are `main`'s own `argv`.

## Reasoning
A UTF-8 active-code-page manifest would leave the "A" calls in place. It needs Windows 10 1903 or
later. It must also be embedded in every executable: the editor, dev, every test and every game a
project builds. With the GNU-driver `clang` that means a resource compiler or `mt` step that the
build does not have today. The "W" calls keep all of this inside the one OS-aware folder. That
folder already uses them for the window title and `CreateProcessW`. `CommandLineToArgvW` is the
system's own reading of the command line, so we do not write a parser for its quoting rules.
Tools the build runs, such as CMake, Ninja and clang, are the programmer's. A non-ASCII project
folder needs them to handle UTF-8 paths too. If one does not, report it; this decision does not
change it.

## Replaces
Nothing. It carries out 0247's "the planner picks the Windows mechanism".
