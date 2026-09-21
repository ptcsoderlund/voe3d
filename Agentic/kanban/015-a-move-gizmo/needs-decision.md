# Needs decision — 015's suite card asks for what ADR-0208 put in 016

## Question
`drive.sh` ran `checks.sh --all` when 015's todo emptied and wrote `blocked/11-suite.md` with 41
findings. None is 015's own: `checks.sh --folder 3d` and `--folder editor` print `FINDINGS: 0`, and
card 10 proved 015 the way ADR-0208 says. The 41 are the older header and entry caps across 14
folders, the whole of work order 016. ADR-0208 exempts 015 from `--all`, but the driver has no
exemption: it runs `--all` with default caps whenever todo, doing and blocked are empty. Replanning
the card into nothing loops the driver back to the same 41 findings; splitting it into cards does
016's work inside 015, against 0208. Which way does 015 end?

## Options
1. **Fold 016 into 015.** The planner splits card 11 into one card per folder (about 16 cards:
   ui, platform, render, text, 3d, editor, dev, ecs, scene, assets, authoring, base, app, theme; the
   largest files, `dev/src/main.c` at 555 header lines and `ui/include/ui/layout.h` at 292, as their
   own cards) with 016's rules; 016 is withdrawn. Replaces 0208. The driver runs unchanged.
2. **Close 015 by hand.** The human takes the walk of card 10 as 015's test, merges 015, and deletes
   `blocked/11-suite.md` with the feature; 016 runs next as planned. 0208 stands; the driver is not
   used to finish 015.
3. **Teach the driver an exemption.** `drive.sh` (in `agentic_rules`) reads a per-feature
   suite command, e.g. a `## Suite` line in `feature.md`, and 015 names `--folder 3d`, `--folder
   editor` and `cmake -P check.cmake`. A workflow change, wider than this repo.

## Recommendation
Option 2. 0208 already decided the order and the reasoning still holds; the suite card has nothing
of 015's in it, and the walk that proves the feature is done. Option 1 is the fallback if the human
wants the driver to be the one that says "you can test".
