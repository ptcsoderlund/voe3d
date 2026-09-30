# game

The loop a shipped game runs: the nineteen types a project's world holds,
one frame drawn through the scene's camera, and the whole run with its sound (0237). A game
tree builds this folder from the engine source through `cmake/game.cmake` (0235),
and a project's `Code/` with it (0242).

- `include` — the public headers, in `include/game/`; each is listed on `include/game/game.md`.
- `src` — the implementation; each file is listed on `src/src.md`.
- `tests` — one plain C program per module, found by the build; each is listed on `tests/tests.md`.
