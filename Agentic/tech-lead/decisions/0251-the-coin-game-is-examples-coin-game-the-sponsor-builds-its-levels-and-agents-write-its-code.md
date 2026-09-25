# 0251 — The coin game is `examples/coin_game`; the sponsor builds its levels and agents write its code
date: 2026-09-25
by: tech-lead

## Decision
The game 0186 aims at lives in the tree as **`examples/coin_game/`**, an example project like
`examples/capsule/` (0244). It is written in **C, the way the engine is**: its own components
and systems (0239), made through work orders, cards and `/drive`, with the same rules and
checks as any engine folder. The work is split by who does it:
- **The sponsor does the level design** in the editor: placing, sizing and colouring entities,
  adding components and setting their values, and saving the scenes.
- **Agents write the code**: the project's `Code/`, and any engine work the game needs.

Agents never edit the sponsor's level scenes. When code needs something in the level (a new
component on the player, a coin's trigger), the work order's How to test says what the sponsor
places, and a card may keep a scene of its own for its checks. A coin game card proves that the
game's code builds and runs, not only that its files are in order (0246 alone is not enough
here); how is the planner's. `examples/capsule/` stays the small showcase of engine features;
`coin_game` is the game.

## Reasoning
A real game built the way the engine is built tests the engine as a developer would meet it, and
keeps the split the sponsor already has: they decide and test, agents build. Keeping level files
the sponsor's alone means an agent's change never overwrites level work. Rejected: growing
`examples/capsule/` into the game (mixes the feature showcase with the game); a game repository
of its own (loses one tree, one set of checks and one `/drive`); agents also building the levels
(the sponsor wants to design them).

## Replaces
nothing. It places milestones 5 and 6 of 0186.
