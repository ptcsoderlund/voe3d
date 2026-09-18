# sprite

A 2D sprite: the one quad every sprite is drawn on, and the materials that read
one cell each out of a sheet. Not billboarding, not animation, and not a
component of its own — a sprite is an ordinary entity wearing this quad and one
of these materials.

- `include` — the public headers, in `include/sprite/`. See
  `include/sprite/sprite.md`.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See
  `tests/tests.md`.
