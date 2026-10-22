# 09 — Loading still runs on the worker pool that 0456 withdrew

## Seen
Cards 48–60 were built on another machine that had not seen 0456, so models load over a pool of worker
threads (0448): `base/workers.h`, `platform/cores.h`, report subjects crossing threads, the store holding the
pool, LODs and pictures made over it, and `game/models_loader.h` loading on a thread beside the frames. I
said loading stays on one thread. The last whole-suite run also failed in this code: the analyser at
`3d/src/model_levels.c:196`, uninitialized arena marks handed to the rewind (`struck/69-suite.md`).

## Expected
As 0457 says: no pool and no loading thread anywhere; the editor and the game load on one thread, a load while
editing between frames as before 0448. What stays: the linear edge weld, a `.glb` read with its pictures left
encoded so Import and the levels make decode none, and the `game: models:` / `game: materials:` time line
with only the steps that remain. Import, LOD and error behaviour stay as 0443–0446 left them. `checks.sh --all`
prints FINDINGS: 0.

## How to reproduce
1. Look for `base/workers.h`, `platform/cores.h` and `game/models_loader.h`: they exist.
2. Open lumberhill in the editor and watch `top`: loading runs on many cores.
3. Run `checks.sh --all`: the analyser finding in `3d/src/model_levels.c` is reported.
