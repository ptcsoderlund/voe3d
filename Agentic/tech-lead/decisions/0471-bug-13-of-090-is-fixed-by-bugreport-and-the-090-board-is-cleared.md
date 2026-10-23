# 0471 — Bug 13 of 090 is fixed by /bugreport, and the leftover 090 board is cleared
date: 2026-10-23
by: tech-lead

## Decision
Bug 13 of 090 (the editor stops responding after the splash while the trees load: a landscape's foliage
kinds and their materials load in the first frame, not under the splash) is fixed with `/bugreport` on the
current branch, outside any feature. 090 stays filed under `Agentic/completed/`, and the board left behind
under `Agentic/kanban/090-…/` is removed so `/drive` sees only 093. If the fix needs more than one folder
or a decision, it comes back to the tech-lead as a work order queued after 093.

## Reasoning
093 was planned (12 cards) before this was answered, and `/drive` runs one feature at a time.
- Reopening 090 would park 093's committed plan for one bug.
- Folding into 093 would mean replanning a committed plan.
- A work order of its own adds renumbering for what is a single startup-loading fix.

## Replaces
nothing
