# 44 — The 3d tests' draw_material.c entry is one sentence
folder: 3d/tests
after: none
decisions: 0168

## Change
In `3d/tests/tests.md` the entry for `draw_material.c` is 362 characters;
the cap is 300. Shorten it to one sentence under 300 characters: an unlit
material's colour on a cube, a model part wearing what its parts set names,
and both sides drawn when double-sided; skips without a graphics card. The
cases cut from the entry (empty or unheld path, a part with no or another
part's naming, the clear colour when one-sided) move into the header comment
of `3d/tests/draw_material.c` if it does not already say them. No code
changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints no FINDING naming `3d/tests/tests.md`.
