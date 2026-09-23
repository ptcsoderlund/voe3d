# platform

The operating system, behind one API. The only folder allowed to include an OS
header, and the only one that knows there is more than one operating system.
Today that is a window, its keyboard and mouse, a clock, reading and writing a
file, and listing, making or finding a folder.

- `include` — the public headers, in `include/platform/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/platform/window.h` — the window API. Opening can fail and returns
  NULL; `_poll` folds events into state; `_close_refuse` takes back a close;
  `_focused` and `_visible` say whether the person is in it and can see it, and
  `_wait` blocks for at most the timeout it is given.
- `include/platform/input.h` — the keyboard and the mouse: keys, motion, the wheel in
  notches, typed UTF-8 text, the pointer's position, buttons and shape, and the lock, which hides the pointer.
  Its header says why it is polled state and a key is a place.
- `include/platform/clock.h` — how long something took. Its header says why it is
  monotonic and not the time of day, why it is seconds as a double, and that
  the wait is on the window.
- `include/platform/file.h` — reading a whole file into an arena, testing whether a
  path is a regular file, and writing a whole file in one call. The file says
  which failure means what, and how a write is made atomic by a `.partial`
  sibling and a rename.
- `include/platform/folder.h` — listing a folder's entries into an arena, making
  one folder, and finding the person's home and this engine's settings folders.
  Its header says what folder and hidden answer, which failure means what, and
  that no path it hands back carries a trailing separator.
- `include/platform/path.h` — joining a folder and a name, a path's parent, its last
  name, and resolving a path to an absolute one. Its header says which separator
  each platform reads and writes, and why a name is a pointer into the path
  itself rather than a copy.
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
- `protocol/fractional-scale-v1.xml` — the scale the compositor would like a
  surface drawn at, in 120ths. Optional; without it the window is stretched.
- `protocol/viewporter.xml` — how a client shows a bigger buffer at the logical
  size. Optional, and only used together with the fractional scale.
- `protocol/cursor-shape-v1.xml` — how a client names the pointer's shape for
  the compositor to draw. Optional; without it the shape does not change.
- `protocol/tablet-v2.xml` — graphics tablets. Vendored only because the cursor
  shape's generated code names its tool type; nothing here uses a tablet.
