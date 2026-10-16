# 0426 — Made levels are kept in the project's Cache and carried by Play and Ship
date: 2026-10-15
by: planner

## Decision
For 089.

1. **One file per `.glb`:** `<folder>/Cache/levels/<model path>.levels`, binary, native-endian: a magic,
   a format version, the FNV-1a 64 hash of the `.glb`'s bytes, whether the file has its own levels,
   the level count, then per made level its error and per part its indices into level 0's baked part.
   A file whose magic, version or hash differs, or that is cut short, is stale and made again; never an
   error. A file with its own levels gets a file of one level, saying so, and is never simplified.
2. **Made by `game`'s loader**, in the editor and the game alike, whenever a `.glb` is loaded and its
   file is missing or stale: the folders made, the file written atomically. A write that fails is one
   line on stderr, and the levels just made are still drawn.
3. **Import makes it under the splash.** After the copy and the materials, a splash wait "Preparing
   <file>" makes the file, so placing the model costs nothing. A re-export the watch finds, or a file
   copied in by hand, is made where it is loaded: under the splash at start and open, between frames
   otherwise, where the reload of a person's own export already waits on the card (0386 allows it).
4. **Play and Ship carry the files** (0375 point 5): `cmake/game.cmake` copies and installs every
   `Cache/levels/**/*.levels` to the same relative path, so the game reads the editor's levels; a
   missing one is made by the game at load.
5. A renamed or deleted model leaves its old file, never read again; `Cache/` may be deleted whole at
   any time (0235).

## Reasoning
Making levels takes a parse, a bake and a simplification: once per file, not per start. A hash of the
bytes and not a stamp, because a copied or checked-out file has a new stamp and the same levels. The
loader every program shares is the one place that knows a `.glb` is about to be drawn.

## Replaces
Nothing.
