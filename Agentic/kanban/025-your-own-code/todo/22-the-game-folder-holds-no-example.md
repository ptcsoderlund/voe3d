# 22 — The engine's `game` folder holds no example
folder: game
decisions: 0168, 0244

## Change
Bug 02, the removal. Card 21 put the example at `examples/capsule/`.

- `game/example/` — removed whole: `git rm -r game/example`, then delete what is left of it on
  disk (its ignored `Build/`).
- `game/game.md` — the `example` entry goes. The opening keeps its points; only if it mentions
  the example, it stops.

## Done when
1. `checks.sh --folder game` prints `FINDINGS: 0`.
2. `test ! -e game/example` exits 0.
3. `git grep -n 'game/example' -- ':!history' ':!Agentic'` prints nothing.
