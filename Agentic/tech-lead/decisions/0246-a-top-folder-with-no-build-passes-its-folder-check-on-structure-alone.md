# 0246 — A top folder with no build passes its folder check on structure alone
date: 2026-09-25
by: tech-lead

## Decision
CLAUDE.md's per-folder check starts with `[ -f "$top/CMakeLists.txt" ] || exit 0;`: a card whose
folder sits under a top folder with no `CMakeLists.txt` (today `examples/`) builds and runs no tests;
it passes on the structure checks (`.md` pages, headers) alone. Engine folders are checked as before.
This unblocks bug 02 of 025: the planner cuts cards in `examples/capsule` (the project, its page
renamed `capsule.md`), `game` (drop `example/` and its line on `game.md`), and rewrites the todo
cards that still name `game/example`.

## Reasoning
One line, keeps the header and `.md` checks on example code, and every later example works
unchanged; the editor builds an example's code itself on Refresh, so no build is lost.
Alternatives: exempt `examples` (drops the structure checks, needs a framework change); move by hand
(fixes this bug only, the next `examples/` card fails the same way).

## Replaces
Nothing. Serves 0244.
