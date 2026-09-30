# 15 — The game tests index entries for steps.c and models.c fit their cap
folder: game/tests
after: none
decisions: 0168

## Change
Only `game/tests/tests.md`, entries `steps.c` (301 characters) and
`models.c` (313); the cap is 300. Shorten each to one sentence naming what
the file tests: for `steps.c` the fixed steps, a body landing, a follower
and an emitter's particles; for `models.c` the model loader on real files,
watches and emitter pictures, skipping without a graphics card. The detail
already lives in the headers of `game/tests/steps.c` and
`game/tests/models.c`; add a point there only if the entry drops something
the header lacks. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints
no FINDING line for `game/tests/tests.md`.
