# platform

The operating system, behind one API. The only folder allowed to include an OS
header, and the only one that knows there is more than one operating system.
Today that is a window and its keyboard and mouse; files and time will live here
too and do not yet.

- `include/platform/window.h` — the window API. Its header carries the two rules
  callers need: opening a window can fail and returns NULL, and `_poll` folds
  events into state rather than handing them out.
- `include/platform/input.h` — the keyboard and the mouse. Its header says why
  this is polled state and not a queue of events, why a key is a place rather
  than a letter, and that a pointer lock is a request with an answer.
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
- `src/library_wayland.c` — dlopen and dlsym. Linux only despite the name; its
  header says why the name says wayland.
- `src/library_win32.c` — LoadLibraryA and GetProcAddress.
- `src/input.h` — the input state both backends fill and neither reads, and the
  two functions each of them defines over its own window. Its header says why
  the public functions are written once.
- `src/input.c` — every function in `include/platform/input.h`, once, for both
  platforms. No `#ifdef` in it and its header says why there must not be one.
- `src/window_wayland.c` — the Linux window, and its seat, keyboard and pointer.
  There is no X11 backend, and nothing in it draws — its header says why the
  window is invisible until something else does, why input is in the same file,
  and why there is no `xkbcommon`.
- `src/window_win32.c` — the Windows window, its keyboard, and the mouse as a raw
  input device. Its header says why `WM_CHAR` and `WM_MOUSEMOVE` are both absent.
- `tests/input.c` — that a poll drains the mouse and keeps held keys, and that
  losing focus releases every key. Needs no window and no display.
