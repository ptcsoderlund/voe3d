# 090 — the board is empty: the feature is filed, bug 13 is not planned

The planner was asked to plan `Agentic/kanban/090-a-forest-is-painted-onto-the-terrain/feature.md`.
That file does not exist. 090 was accepted and filed to `Agentic/completed/090-…/` (commit 16bcb6f3);
what is left under `Agentic/kanban/090-…/` is five empty columns, which `drive.sh` takes for an open
feature with no cards and so asks for a plan of its `feature.md`.

Filed with it, and not planned, is
`Agentic/completed/090-…/bugs/13-the-editor-stops-responding-after-the-splash-while-the-trees-load.md`:
a landscape's foliage kinds and their materials load in the first frame, not under the splash. No work
order carries it (093 names only the picture decode as its own).

## Question
Where does bug 13 get planned, and what happens to the empty 090 board?

## Options
1. Reopen 090: `git mv` the completed folder back to `Agentic/kanban/`, so `drive.sh` plans bug 13 on
   `feature/090-…` (the branch exists and sits on dev's head) and files 090 again after it.
2. A work order of its own, queued before 093 (renumbering 093–099), and delete the empty 090 board so
   `drive.sh` takes the next order from dev.
3. Fold bug 13 into work order 093, which already reworks what loads under the splash; delete the empty
   090 board.

## Recommendation
1. The fault is 090's own delivery (the foliage kinds the splash does not count), the bug file and the
   done cards a planner needs sit together in 090's folder, and the bug flow exists for this. It adds no
   renumbering and keeps 093 about the cache.
