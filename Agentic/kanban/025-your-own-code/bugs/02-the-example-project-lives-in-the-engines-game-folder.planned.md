# 02 — The example project lives in the engine's `game` folder

## Seen
The example project is at `game/example/`, inside the engine folder that holds the game loop. A
newcomer does not look for examples there, and `game` is also the name of a project's output
(`<project>/Build/game/`).

## Expected
The example project is at `examples/capsule/` at the repository root, and nothing of it is left under
`game/`. Everything that names its old place (feature.md's How to test, folder pages, comments)
names the new one. Opening `examples/capsule/project.voe3d` in the editor works exactly as the old
path did. Decision 0244.

## How to reproduce
1. Look at the repository root: there is no `examples/`.
2. The example is found only at `game/example/project.voe3d`.
