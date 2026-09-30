# 14 — The 3d tests index entry for models.c fits its cap
folder: 3d/tests
after: none
decisions: 0168

## Change
Only `3d/tests/tests.md`, entry `models.c`. It is 342 characters; the cap is
300. Shorten it to one sentence naming what the file tests (the model store:
loads, failures, replace, clear, pictures and the dot, a model drawn). The
detail already lives in the header of `3d/tests/models.c`; read that header
only if the entry drops something it lacks, and then add the point there.
No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints no
FINDING line for `3d/tests/tests.md`.
