# 0341 — The root folder passes its folder check; the suite checks it
date: 2026-10-03
by: tech-lead

## Decision
The per-folder check line in `CLAUDE.md` `## Checks` exits 0 for the repository root (`folder: .`):
its guard is `[ "$top" != . ] && [ -f "$top/CMakeLists.txt" ] || exit 0`. A card whose folder is `.`
(such as one editing `check.cmake`) is therefore not built per folder; the root's files are checked by
the whole-suite `cmake -P check.cmake`, which `/drive` runs once after a feature's last card. The tech-lead
made the edit; no card changes `CLAUDE.md`. With this, 052 bug 04 is planned as 0340 says: the `base`
fix, the parts of `check.cmake` in a new `check/` folder, `check.cmake` including them, then the Release
step.

## Reasoning
The old line built `voe_.`, which does not exist, so every root card failed its check and blocked.
Alternatives: the first root card edits `CLAUDE.md` itself (same result, but an agent edits the human's
instructions file); only the `base` fix now (the Release step of 0340 waits, and the next Release-only
break is again found by Ship).

## Replaces
Nothing. Carries out 0340.
