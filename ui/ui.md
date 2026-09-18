# ui

Nested rows and columns of boxes in millimetres, a rectangle for every one of
them, and the first widgets on top: a panel, a label, a button that answers the
mouse, a number box you drag sideways to change a value, a single-line text
field, an image and a scroll area that remembers its offset. Not drawing — what
comes out is element records and the caller submits them — and not input
either: the pointer and the keyboard are values it is handed. Where the surface
sits in the world is one matrix and it is the caller's.

- `include` — the public headers, in `include/ui/`. See `include/ui/ui.md`.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See
  `tests/tests.md`.
