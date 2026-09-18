# platform

The operating system, behind one API. The only folder allowed to include an OS
header, and the only one that knows there is more than one operating system.
Today that is a window, its keyboard and mouse, a clock, reading and writing a
file, and listing, making or finding a folder.

- `include` — the public headers, in `include/platform/`; each is listed below by path.
- `src` — the implementation; each file is listed below by path.
- `tests` — one plain C program per module, found by the build; each is listed below by path.
- `include/platform/window.h` — the window API. Its header carries the two rules
  callers need: opening a window can fail and returns NULL, and `_poll` folds
  events into state rather than handing them out; `_close_refuse` takes back a
  close the user just asked for.
- `include/platform/input.h` — the keyboard and the mouse: keys, motion, the
  wheel in notches, typed UTF-8 text, the pointer's position and buttons, and
  the lock. Its header says why this is polled state and not a queue of events,
  why a key is a place rather than a letter, why typed text is the one
  exception with order in it and is still not a queue, why motion and position
  are two questions and which is for what, when a position is not live, that a
  pointer lock is a request with an answer, and what sign a notch has.
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
- `src/keymap.h` — the XKB keymap reader: an evdev-code, four-level table and
  an AltGr flag per code, built from resolved XKB v1 text, internal to this
  folder and built on both platforms because it includes no OS header. Its
  header says which two blocks it reads, which two spellings of a key's
  symbols it reads, how a keysym as a number differs from one as a name, what
  makes a key AltGr, what it does not read, and when it refuses a keymap
  outright.
- `src/keymap.c` — its implementation: one lexer and a small parsing function
  per level of the format's fixed nesting, none of them calling itself. Its
  header says why a brace counter stands in for recursion here, and why a key
  body is read by its statements rather than by its first `[`.
- `src/input.h` — the input state both backends fill and neither reads, the two
  functions each of them defines over its own window, the three clears both of
  them call, and the one shared function that turns a code point into the
  typed-text buffer's UTF-8. Its header says why the public functions are
  written once.
- `src/input.c` — every function in `include/platform/input.h`, once, for both
  platforms. No `#ifdef` in it and its header says why there must not be one.
- `src/window_wayland.c` — the Linux window, and its seat, keyboard and pointer.
  There is no X11 backend, and nothing in it draws — its header says why the
  window is invisible until something else does, why input is in the same file,
  why the keymap is read in-house rather than through `xkbcommon`, and why
  AltGr is held like Shift rather than read from the compositor's modifiers.
- `src/window_win32.c` — the Windows window, its keyboard and its `WM_CHAR`
  text, the mouse as a raw input device for look and as ordinary messages for
  position and buttons. Its header says why Control without Alt drops
  everything `WM_CHAR` sends, why AltGr still types, why a held button takes
  the capture, and why `WM_MOUSELEAVE` has to be asked for.
- `tests/input.c` — that a poll drains the mouse's motion, wheel and typed text
  and keeps held keys and the pointer, that losing focus releases every key and
  its typed text and nothing else, that losing the pointer releases every
  button and keeps its last position, that a code point past capacity is
  dropped whole, and that a control code point types nothing. Needs no window
  and no display.
- `tests/keymap.c` — a hand-written keymap text checked key by key: an
  ordinary key, one with a type statement beside its `symbols[Group1]`, a
  digit and its shifted punctuation, a key with one level, a dead key that
  types nothing, a `U`-named code point, an alias, and that text with no
  `xkb_symbols` block is refused. A second text, copied from a real
  compositor's own spelling, checks `0x` keysym values, a Group index written
  `1`, an unindexed `type=`, AltGr as the keysym `0xfe03` on two keys, the
  `0x0100xxxx` Unicode form, a key with no resolved code, and that a keymap
  resolving to nothing is refused too. Needs no window and no display.
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
