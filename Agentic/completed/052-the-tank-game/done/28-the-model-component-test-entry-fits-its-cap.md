# 28 — The model component test's index entry fits its cap
folder: 3d/tests
after: none
decisions: 0168

## Change
`3d/tests/tests.md`: the `model_component.c` entry is 305 characters, the cap is 300. Shorten it
to one sentence well under the cap: the model component's fields, its default and intents read
back after a run, a dead entity's intent dropped and a long path cut, no graphics card needed.
Drop the field-by-field detail (byte size, field kinds, the 0.5 fade); the header of
`3d/tests/model_component.c` already says all of it. Read that header only to confirm nothing the
entry drops is missing from it; if something is, add it there as a phrase. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints no FINDING naming
`3d/tests/tests.md`.
