# platform

The operating system, behind one API. The only folder allowed to include an OS
header, and the only one that knows there is more than one operating system.
Today that is a window; input, files and time will live here too and do not yet.

- `include/platform/window.h` — the window API. Its header carries the two rules
  callers need: opening a window can fail and returns NULL, and `_poll` folds
  events into state rather than handing them out.
- `protocol/xdg-shell.xml` — the Wayland shell protocol, vendored.
  `wayland-scanner` turns it into C at build time; nothing generated is
  committed.
- `src/window_wayland.c` — the Linux window. There is no X11 backend.
- `src/window_win32.c` — the Windows window.
- `dev/window.c` — opens one and prints its size. Built by an ordinary build,
  never run by the check script, and the only verification this folder has.
