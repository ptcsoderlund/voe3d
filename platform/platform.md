# platform

The operating system, behind one API. The only folder allowed to include an OS
header, and the only one that knows there is more than one operating system.
Today that is a window, its keyboard and mouse, a clock, reading and writing a
file, and listing, making or finding a folder.

- `include/platform/window.h` — the window API. Its header carries the two rules
  callers need: opening a window can fail and returns NULL, and `_poll` folds
  events into state rather than handing them out.
- `include/platform/input.h` — the keyboard and the mouse: keys, motion, the
  wheel in notches, the pointer's position and buttons, and the lock. Its header
  says why this is polled state and not a queue of events, why a key is a place
  rather than a letter, why motion and position are two questions and which is
  for what, when a position is not live, that a pointer lock is a request with
  an answer, and what sign a notch has.
- `include/platform/clock.h` — how long something took. Its header says why it is
  monotonic and not the time of day, why it is seconds as a double, and that
  waiting is a different question this folder cannot answer yet.
- `include/platform/file.h` — reading a whole file into an arena, testing
  whether a path is a regular file, and writing a whole file in one call. Its
  header says why files are this folder's, that the path is exactly what the
  caller gave, what an empty file reads as, which failure means what, and how
  a write is made atomic by a `.partial` sibling and a rename.
- `include/platform/folder.h` — listing a folder's entries into an arena, making
  one folder, and finding the person's home and this engine's settings folders.
  Its header says what folder and hidden answer, which failure means what, and
  that no path it hands back carries a trailing separator.
- `include/platform/path.h` — joining a folder and a name, a path's parent, its
  last name, and resolving a path to an absolute one. Its header says which
  separator each platform reads and writes, that a root's parent is NULL, and
  why a name is a pointer into the path itself rather than a copy.
- `include/platform/library.h` — a shared library opened by name at run time, and
  a symbol out of it. Its header says why this is here and not in the folder that
  wants one.
- `protocol/xdg-shell.xml` — the Wayland shell protocol, vendored.
  `wayland-scanner` turns every XML here into C at build time; nothing generated
  is committed.
- `protocol/xdg-decoration-unstable-v1.xml` — how a client asks the compositor
  to draw the window frame. Optional, and the answer may be no.
- `protocol/relative-pointer-unstable-v1.xml` — how far the mouse moved, as
  opposed to where the pointer is. Optional; mouse look is what is lost without
  it.
- `protocol/pointer-constraints-unstable-v1.xml` — how a client asks for the
  pointer to stop going anywhere. Optional, and the compositor may say no.
- `src/clock_wayland.c` — CLOCK_MONOTONIC. Linux only despite the name, the same
  as the file below; its header says why the adjusted clock and not the raw one.
- `src/clock_win32.c` — the performance counter, and the frequency asked for once.
  Its header says why not the millisecond tick counts.
- `src/file_wayland.c` — open, read/write, close, and the rename that makes a
  write atomic. Linux only despite the name; its header says why the name says
  wayland, why the read and write loops and the checked fsync and close are
  not optional, and why the `.partial` path is a stack buffer.
- `src/file_win32.c` — CreateFileA, ReadFile/WriteFile, CloseHandle, and the
  MoveFileExA that makes a write atomic. Its header says why the ASCII call,
  why a 64-bit count is moved in steps, and why the `.partial` path is a stack
  buffer.
- `src/folder_wayland.c` — opendir/readdir, mkdir, and $HOME/$XDG_CONFIG_HOME.
  Linux only despite the name; its header says why listing is two passes and
  the sort is by hand, and when `fstatat` decides `folder` instead of `d_type`.
- `src/folder_win32.c` — FindFirstFileA/FindNextFileA, CreateDirectoryA, and
  GetEnvironmentVariableA for %USERPROFILE%/%APPDATA%. Its header says why a
  directory symlink needs no extra call here, unlike the Linux side.
- `src/path.c` — joining, finding a parent and finding a name, for both
  platforms. Its header says why the separator is one compile-time constant
  rather than an `#ifdef` in each function.
- `src/path_wayland.c` — realpath, resolving a path to an absolute one. Linux
  only despite the name; its header says why `PATH_MAX` is realpath's own
  buffer and not a limit this folder invents.
- `src/path_win32.c` — GetFullPathNameA plus GetFileAttributesA, since the
  first alone will invent a path for a name that is not there. Its header
  says why two calls size the buffer.
- `src/library_wayland.c` — dlopen and dlsym. Linux only despite the name; its
  header says why the name says wayland.
- `src/library_win32.c` — LoadLibraryA and GetProcAddress.
- `src/input.h` — the input state both backends fill and neither reads, the two
  functions each of them defines over its own window, and the three clears both
  of them call. Its header says why the public functions are written once.
- `src/input.c` — every function in `include/platform/input.h`, once, for both
  platforms. No `#ifdef` in it and its header says why there must not be one.
- `src/window_wayland.c` — the Linux window, and its seat, keyboard and pointer.
  There is no X11 backend, and nothing in it draws — its header says why the
  window is invisible until something else does, why input is in the same file,
  and why there is no `xkbcommon`.
- `src/window_win32.c` — the Windows window, its keyboard, the mouse as a raw
  input device for look and as ordinary messages for position and buttons. Its
  header says why `WM_CHAR` is absent, why a held button takes the capture, and
  why `WM_MOUSELEAVE` has to be asked for.
- `tests/input.c` — that a poll drains the mouse's motion and wheel and keeps
  held keys and the pointer, that losing focus releases every key and nothing else, and
  that losing the pointer releases every button and keeps its last position.
  Needs no window and no display.
- `tests/file.c` — that the bytes written come back byte for byte on both sides
  of the API, that a shorter file replaces a longer one, that an empty file
  reads as zero bytes, that a missing path or a folder fails a read as
  UNAVAILABLE, that no `.partial` sibling outlives a successful write, and
  that writing to a path that is itself a folder fails the rename as REFUSED
  without touching the folder. Reads and writes with stdio as the oracle on
  purpose; needs no window and no display.
- `tests/clock.c` — that the clock moves and never goes backwards. Its header
  says why nothing in it measures a duration against a duration.
- `tests/folder.c` — that a listing is sorted by byte order with folder and
  hidden answered correctly, that an empty folder lists zero, that a missing
  folder fails a listing as UNAVAILABLE, that creating over an existing name is
  REFUSED and under a missing parent UNAVAILABLE, and that the settings folder
  honours `XDG_CONFIG_HOME` on Linux. Needs no window and no display.
- `tests/path.c` — join, parent and name on ordinary paths, roots and trailing
  separators, for the platform it runs on; that resolving "." to an absolute
  path is idempotent, and that resolving a made-up name is NULL. Needs no
  window and no display.
