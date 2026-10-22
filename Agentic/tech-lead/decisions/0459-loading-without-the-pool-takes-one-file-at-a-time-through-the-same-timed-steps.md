# 0459 — Loading without the pool takes one file at a time through the same timed steps
date: 2026-10-22
by: planner

## Decision
How 0457 is carried out, for 090 bug 09.

1. **The steps stay; the pool goes.** The game's batch keeps its six steps (reading, parsing, making
   LODs, decoding pictures, baking, uploading) and 3d keeps the prepare and put split, the material
   put from decoded maps and the public picture decode: they are where the steps are timed. Each
   step runs its files and pictures in order on the calling thread. A batch holds one file, a
   materials chunk one material, so a load holds one file's arenas at a time, as before 0448. The
   `game: models:` line keeps all six steps, `game: materials:` its three: none of them is gone.
2. **The levels make goes level by level.** Each made level's parts are simplified in order onto the
   caller's arena and the level judged against the one before as soon as it is made: the first not
   below it is given back to the arena and ends the list (0436 point 5), as before 090 card 53. The
   `.glb` is still read with its pictures left encoded.
3. **The editor loads between frames whole.** The update and the once-a-second watch run between
   two frames as they did before 090 card 60; a splash wait's call is unchanged.
4. **Nothing carries threads for loading.** `base/workers.h`, base's carried report state,
   `platform/cores.h` and `game/models_loader.h` are deleted with their tests and entries; the
   store holds no pool; no loading signature takes one.

## Reasoning
The smallest removal that keeps 0457's three gains. Alternatives: going back to the game loader
before 0448 (loses the time line 0457 keeps, and the encoded read's callers); keeping chunks of
several files on one thread (more memory, no speed). The level-by-level make holds one level's
buffers at most and drops the arena marks the analyser flagged in struck card 69.

## Replaces
Nothing. Carries out 0457.
