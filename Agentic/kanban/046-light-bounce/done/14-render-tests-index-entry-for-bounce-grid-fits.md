# 14 — The render tests index entry for bounce_grid.c fits its cap
folder: render/tests
after: none
decisions: 0168

## Change
Documentation only; no test code changes.

`render/tests/tests.md`: the entry for `bounce_grid.c` is 305 characters,
the cap is 300. Cut it to one sentence well under 300 characters: every
target's probe grid built, kept and freed, and the update's refusals; headless.

`render/tests/bounce_grid.c`, header comment only: make sure the details the
entry drops are made there — the window's and two targets' grids, kept
through a resize and freed with the device twice, each still drawing a red
cube; the update refused with no bounce pass, inside a pass, twice a frame
and for a stale id, else named by a camera pass. Add only what is missing.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `render/tests/tests.md`.
