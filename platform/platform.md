# platform

The operating system, behind one API. The only folder allowed to include an OS
header, and the only one that knows there is more than one operating system.
Today that is a window, its keyboard and mouse, a clock, reading and writing a
file, and listing, making or finding a folder.

- `include` — the public headers, in `include/platform/`. See
  `include/platform/platform.md`.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See
  `tests/tests.md`.
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
