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
- `file_win32.c` — CreateFileW, GetFileAttributesW, ReadFile/WriteFile, CloseHandle, and the
  MoveFileExW that makes a write atomic, a UTF-8 path converted on the way in. Its header says why
  a 64-bit count is moved in steps, and why a path too long for its stack buffer fails as an open.
- `folder_wayland.c` — opendir/readdir, mkdir, and $HOME/$XDG_CONFIG_HOME. Its
  header says why listing is two passes and the sort is by hand, and when
  `fstatat` decides `folder` instead of `d_type`.
- `folder_win32.c` — FindFirstFileW/FindNextFileW, CreateDirectoryW, and
  GetEnvironmentVariableW for %USERPROFILE%/%APPDATA%, each name made UTF-8. Its header says why a
  directory symlink needs no extra call here, unlike the Linux side.
- `wide_win32.h` — UTF-8 to UTF-16 into a caller's buffer and UTF-16 to UTF-8 into an arena, for
  the Windows files here only. Its header says why the "W" calls and not a manifest, and why a path
  that does not fit is the caller's ordinary failure.
- `wide_win32.c` — its implementation, with flags 0 so malformed text becomes U+FFFD.
- `path.c` — joining, finding a parent and finding a name, for both platforms.
  Its header says why the separator is one compile-time constant rather than an
  `#ifdef` in each function.
- `path_wayland.c` — realpath, resolving a path to an absolute one, and readlink on
  `/proc/self/exe` for the program's own path. Its header
  says why `PATH_MAX` is realpath's own buffer and not a limit this folder
  invents.
- `path_win32.c` — GetFullPathNameW plus GetFileAttributesW, since the first
  alone will invent a path for a name that is not there, the result made UTF-8; GetModuleFileNameW, grown until not truncated,
  for the program's own path. Its header says why two calls size the buffer.
- `library_wayland.c` — dlopen and dlsym, a failed open reported with dlerror. Its header says why
  the name says wayland.
- `library_win32.c` — LoadLibraryW and GetProcAddress, a failed open reported through
  FormatMessageW made UTF-8.
- `sound_wayland.c` — ALSA's default device through libasound loaded at run time, its calls
  declared here with no ALSA header. Its header says why the stream is started by hand.
- `sound_win32.c` — WASAPI in shared mode through COM in C, ole32 loaded at run time and the
  GUIDs defined here. Written, not verified.
- `process_wayland.c` — posix_spawnp into a new process group with output dup2'd onto a file when
  asked, waitpid to poll, SIGTERM to the group to end.
- `process_win32.c` — the argument list quoted into one command line, CreateProcessW inside a
  kill-on-close job object with an inheritable CreateFileW output handle when asked, and
  TerminateJobObject to end.
- `arguments_wayland.c` — main's `argv`, handed back as it is.
- `arguments_win32.c` — GetCommandLineW and CommandLineToArgvW, each entry made UTF-8 into the
  arena, and `argv` as the fallback when the split fails.
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
- `window_wayland.c` — the Linux window: registry, shell, decoration, fractional scale, cursor
  shape manager, open, close and poll; there is no X11 backend, and nothing in it draws.
- `seat_wayland.c` — the Linux window's seat: keyboard, pointer, its shape by name, relative
  motion and the lock. Its header says why it is a file of its own and when a locked pointer is hidden.
- `scale.h` — logical Wayland units to buffer pixels at a scale in 120ths, a
  length rounded half away from zero and a position not rounded. OS-free like
  `input.h`, so it is built on both platforms and tested without a compositor.
- `scale.c` — its implementation, in 64-bit integers for the length.
- `window_win32.h` — the Windows window's struct, shared by the two files below and seen by
  nothing outside this folder, and the seat functions the window procedure calls.
- `window_win32.c` — the Windows window: its class, the window procedure, open, close, poll, wait
  and the queries.
- `seat_win32.c` — the Windows window's seat: the keyboard and its `WM_CHAR` text, the mouse as a
  raw input device for look and as ordinary messages for position and buttons, its shape, and the
  lock.
