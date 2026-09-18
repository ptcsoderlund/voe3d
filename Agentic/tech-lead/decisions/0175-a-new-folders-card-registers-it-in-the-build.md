# 0175 — A new folder's card registers it in the build
date: 2026-09-18
by: tech-lead

## Decision
The build is the coder's, like the rest of the code. A card that creates a code folder also registers it, in the
same commit: the folder's row in `cmake/voe.cmake` and its one `add_subdirectory` line in the root
`CMakeLists.txt`. A card that gives a folder a new dependency adds it to that folder's row in `cmake/voe.cmake`.
That card belongs to the folder whose row changes; for 006 that is the `editor/` card that adds `theme`. The card
names the decision that grants the edge, as 0170 does for `theme`. These two files are the only root build files a
card may touch this way. `checks.sh --folder` must allow them when the card in `doing/` has a `decisions:` line,
the same test it already applies to dependency manifests. That allowance is made in `~/Projekt/agentic_rules` and
installed with `install.sh`. It is a coder's task, and it comes first, before 006's card 05 is re-cut and built.

## Reasoning
The registration lands in the same commit as the folder, so the tree builds at every commit, and every future
folder is covered.
- Tech-lead makes the build edits and a stub folder: puts code outside any card and outside the coder's hands.
- The root build globs folders: reverses step 1b of `check.cmake` and still leaves the `voe.cmake` row unsolved.

## Replaces
nothing
