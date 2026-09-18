# platform

The operating system, behind one API. The only folder allowed to include an OS
header, and the only one that knows there is more than one operating system.
Today that is a window, its keyboard and mouse, a clock, reading and writing a
file, and listing, making or finding a folder.

- `include` — the public headers, in `include/platform/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
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
