# Needs decision — how a card in `examples/` passes its folder check

Bug 02 (decision 0244) moves the example to `examples/capsule/`. A coder proves a card with
`checks.sh --folder <folder>`, and that runs CLAUDE.md's per-folder check with `top=examples`:
`cmake --build --preset debug --target voe_examples ...`. No such target exists (0244: `examples/`
has no target and no tests), so the check fails on every card whose folder is under `examples/`,
now and for every later example. A card in `game/` cannot write `examples/` either: `--folder`
reports any change outside the card's folder. Changing the per-folder check is repo-wide, so it
is not the planner's.

## Options
1. **The per-folder check skips a top folder with no build.** CLAUDE.md's first `## Checks` line
   starts with `[ -f "$top/CMakeLists.txt" ] || exit 0;`. A data folder passes on the structure
   checks alone (its `.md` pages and headers); engine folders are checked as now. The human makes
   the one-line edit, then the planner cuts the cards: `examples/capsule` (the project, its page
   renamed `capsule.md`), `game` (remove `example/`, its line on `game.md`), and the todo cards
   that still name `game/example` (16, 19, 20) rewritten to the new path.
2. **`examples` goes under CLAUDE.md's `## Exempt`, and `checks.sh --folder` skips the product's
   commands for an exempt folder.** A framework change in `agentic_rules` too. The downside: exempt
   also drops the structure checks, so `examples/capsule/Code` gets no header or `.md` checks.
3. **No card: the human or tech-lead makes the move as one commit by hand** (`git mv`, two `.md`
   edits, the three todo cards). It fixes this bug only; the next card touching `examples/` hits
   the same failing check.

## Recommendation
Option 1: a one-line edit, keeps the structure checks on the example's C files, and every later
example works with no further change.
