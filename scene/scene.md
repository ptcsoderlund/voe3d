# scene

Where things are, what looks at them and what lights them: the components a
person would author, and the systems that own them. Nothing here names a GPU
resource, a file or a graphics API — a projection matrix is `3d`'s, because clip
space is.

- `include` — the public headers, in `include/scene/`. See
  `include/scene/scene.md`.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See
  `tests/tests.md`.
