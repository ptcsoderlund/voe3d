# src

`platform`'s implementation. Every public function is here twice, once per
operating system, in a file named for the backend it belongs to — `_wayland` is
Linux and `_win32` is Windows — except where one file can serve both, and then
there is only one. This is the only folder in the engine that includes an OS
header.

- `clock_wayland.c` — CLOCK_MONOTONIC. Linux only despite the name, as every
  `_wayland` file here is; its header says why the adjusted clock and not the
  raw one.
- `clock_win32.c` — the performance counter, and the frequency asked for once.
  Its header says why not the millisecond tick counts.
- `file_wayland.c` — open, read/write, close, and the rename that makes a write
  atomic. Its header says why the name says wayland, why the read and write
  loops and the checked fsync and close are not optional, and why the
  `.partial` path is a stack buffer.
- `file_win32.c` — CreateFileA, ReadFile/WriteFile, CloseHandle, and the
  MoveFileExA that makes a write atomic. Its header says why the ASCII call,
  why a 64-bit count is moved in steps, and why the `.partial` path is a stack
  buffer.
- `folder_wayland.c` — opendir/readdir, mkdir, and $HOME/$XDG_CONFIG_HOME. Its
  header says why listing is two passes and the sort is by hand, and when
  `fstatat` decides `folder` instead of `d_type`.
- `folder_win32.c` — FindFirstFileA/FindNextFileA, CreateDirectoryA, and
  GetEnvironmentVariableA for %USERPROFILE%/%APPDATA%. Its header says why a
  directory symlink needs no extra call here, unlike the Linux side.
- `path.c` — joining, finding a parent and finding a name, for both platforms.
  Its header says why the separator is one compile-time constant rather than an
  `#ifdef` in each function.
- `path_wayland.c` — realpath, resolving a path to an absolute one. Its header
  says why `PATH_MAX` is realpath's own buffer and not a limit this folder
  invents.
- `path_win32.c` — GetFullPathNameA plus GetFileAttributesA, since the first
  alone will invent a path for a name that is not there. Its header says why two
  calls size the buffer.
- `library_wayland.c` — dlopen and dlsym. Its header says why the name says
  wayland.
- `library_win32.c` — LoadLibraryA and GetProcAddress.
- `keymap.h` — the XKB keymap reader: an evdev-code, four-level table and an AltGr flag per code,
  built from resolved XKB v1 text, internal to this folder and built on both platforms because it
  includes no OS header.
- `keymap.c` — its implementation: one lexer and a small parsing function per
  level of the format's fixed nesting, none of them calling itself. Its header
  says why a brace counter stands in for recursion here, and why a key body is
  read by its statements rather than by its first `[`.
- `input.h` — the input state both backends fill and neither reads, the two functions each of them
  defines over its own window, the three clears both of them call, and the one shared function that
  turns a code point into UTF-8.
- `input.c` — every function in `include/platform/input.h`, once, for both
  platforms. No `#ifdef` in it and its header says why there must not be one.
- `window_wayland.h` — the Linux window's struct, shared by the two files below and seen by
  nothing outside this folder, and the two functions that cross between them.
- `window_wayland.c` — the Linux window: registry, shell, decoration, fractional scale, open,
  close and poll; there is no X11 backend, and nothing in it draws.
- `seat_wayland.c` — the Linux window's seat: keyboard, pointer, relative motion and the lock.
  Its header says why it is a file of its own and why the cursor image is untouched.
- `scale.h` — logical Wayland units to buffer pixels at a scale in 120ths, a
  length rounded half away from zero and a position not rounded. OS-free like
  `input.h`, so it is built on both platforms and tested without a compositor.
- `scale.c` — its implementation, in 64-bit integers for the length.
- `window_win32.c` — the Windows window, its keyboard and its `WM_CHAR` text, the mouse as a raw
  input device for look and as ordinary messages for position and buttons.
