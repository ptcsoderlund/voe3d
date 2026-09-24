# 0235 — Play builds the game in the project folder, from the engine's source
date: 2026-09-24
by: tech-lead

## Decision
Play (0187) builds the game in a **build tree of its own inside the project folder**, with the engine
compiled from its source. The editor knows where that source is. The editor writes the tree and runs
the build itself, and neither the tree nor the build touches this repository's build. The game is
built the way a shipped game is built, so it has no field descriptions (ADR-0145 point 3) and does
not link `authoring` (ADR-0151). Milestone 3 adds the project's own C to this same tree, and
milestone 6 builds the same tree in release. The first Play in a project compiles the engine once,
which is accepted as long as the editor shows that it is building. Every later Play rebuilds only
what changed. Anything generated goes in **separate top-level folders that git ignores as whole
folders**: `Build/` for the game's build tree and `Cache/` for any other generated data. Folders a
developer writes in, such as `Assets/`, are tracked, and nothing generated is ever written into
them. So a project's `.gitignore` lists only folders, never single files. Play builds with
**debug info**, so a developer can attach any debugger to the running game, or open `Build/` in an
IDE and start the game from there. The editor offers no debugger of its own for now. There is
**one cook**. Play and the shipped build turn a world into the same output, and they differ only
in debug vs release and in Play taking the unsaved scene (0188). The planner picks the new
engine folders the cook and the game loop need, and where they sit in the folder order (rule 2).

## Reasoning
This is already the shape milestones 3 and 6 need, and it keeps the game free of editor-only code.
Rejected alternatives:
- building the game as a target in the editor's own build: it only works from a checkout, puts
  descriptions in the game, and mixes projects into the engine's build.
- a player program that reads the scene text: this contradicts 0187 and ADR-0151, and milestone 3
  throws it away.
- one shared engine build per editor: this saves only each project's first Play, and it has to be
  kept in step with each editor version. It is noted in ideas.md.

Two cooks would let a game work in Play and break once shipped. A hidden `.voe3d/` folder was rejected in favour of visible, named folders, so it is obvious what
is generated and what is the developer's.

## Replaces
nothing
