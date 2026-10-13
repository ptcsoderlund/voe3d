# 46 — The game tests' models.c entry is one sentence
folder: game/tests
after: none
decisions: 0168

## Change
In `game/tests/tests.md` the entry for `models.c` is 358 characters; the
cap is 300. Shorten it to one sentence under 300 characters: the model loader
on real files, its main claims grouped (a `.glb` loaded, missing and
watched; landscapes and their tables; a model's parts wearing table
materials), skips without a graphics card. The full list of cases belongs in
the header comment of `game/tests/models.c`; add any missing there, keeping
it within 60 lines (measure with `bash ~/.claude/skills/checks/scripts/checks.sh --header-lines game/tests/models.c`).
No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints no FINDING naming `game/tests/tests.md`.
