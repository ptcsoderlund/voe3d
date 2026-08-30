# platform

The operating system, behind one API. The only folder allowed to include an OS
header, and the only one that knows there is more than one operating system.
Today that is a window; input, files and time will live here too and do not yet.

- `include/platform/window.h` — the window API. Its header carries the two rules
  callers need: opening a window can fail and returns NULL, and `_poll` folds
  events into state rather than handing them out.
- `include/platform/library.h` — a shared library opened by name at run time, and
  a symbol out of it. Its header says why this is here and not in the folder that
  wants one.
- `protocol/xdg-shell.xml` — the Wayland shell protocol, vendored.
  `wayland-scanner` turns every XML here into C at build time; nothing generated
  is committed.
- `protocol/xdg-decoration-unstable-v1.xml` — how a client asks the compositor
  to draw the window frame. Optional, and the answer may be no.
- `src/library_wayland.c` — dlopen and dlsym. Linux only despite the name; its
  header says why the name says wayland.
- `src/library_win32.c` — LoadLibraryA and GetProcAddress.
- `src/window_wayland.c` — the Linux window. There is no X11 backend, and nothing
  in it draws — its header says why the window is invisible until something else
  does.
- `src/window_win32.c` — the Windows window.
