# 0392 — 082 bug 05 is work order 083, built right after 082
date: 2026-10-08
by: tech-lead

## Decision
Carrying out 0391: 082 blocked a fourth time (card 65: a whole relight already keeps the tint; the failing
check, `3d/tests/bounce_scene.c:566`, reads after a blocker is added and removed), so bug 05 leaves 082.
Its bug report, blocked card 65 and todo card 66 are removed from 082's board, and 082 goes on to its
suite and the sponsor's test without the tint. Bug 05 becomes work order 083, "A small coloured box tints
the ground beside it", and is built next, before the rest of the hill. 0376's and 0377's 083–092 are
renumbered 084–093 in the same order. 0387, 0389 and 0390 stand as how it is built. 083's planner starts
from the commits on `feature/082-a-terrain-sculpted-with-brushes` (cards 41–58 done, card 65's `## Blocked`
in commit 354a8dad and the replans before it), not from scratch.

## Reasoning
The sponsor's call (2026-10-08): right after 082. Every later hill feature (sky, forest, grass) lights its
world with the bounce, so building on a stale one would spread the fault. Fixing it while the nests are fresh
is cheapest. The fault is narrowed to the add-then-remove path of a blocker, which is small enough for one
work order.
- After 087 (sky): sky light changes the bounce anyway, but the fault would sit under three features.
- Last, before 093: later features might be built on a wrong bounce.

## Replaces
Nothing. Carries out 0391; amends 0376's and 0377's numbering of the hill's work orders.
