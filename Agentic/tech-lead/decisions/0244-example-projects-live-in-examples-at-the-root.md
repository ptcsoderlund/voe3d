# 0244 — Example projects live in `examples/` at the root
date: 2026-09-25
by: tech-lead

## Decision
Example projects are data folders under `examples/` at the repository root, one folder per project
named for what it shows. 025's example moves from `game/example/` to `examples/capsule/`, with its
page renamed to match its folder. The engine folder `game/` keeps its name and its code; it holds no
example. `examples/` is not an engine folder: it has no target and no tests of its own.

## Reasoning
The human wants onboarding to read well: a newcomer looks for examples at the root under the name
everyone uses, not inside an engine folder whose name also appears as a project's output
(`<project>/Build/game/`). Alternatives: `editor_examples/` (long, and the projects run as games
too); `editor/examples/` (example data inside a code folder with its own checks and caps).

## Replaces
Amends 0242 point 10 and its rejection of `examples/` at the root.
