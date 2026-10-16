# 44 — The game loader's test entry is one sentence
folder: game/tests
after: none
decisions: 0168

## Change
`game/tests/tests.md`: the entry for `models.c` is 341 characters, cap 300.
Cut it to one sentence under 270 characters: the model loader on real files,
a `.glb`'s levels file among them, skipping without a graphics card. Details
that may be dropped (a `.glb` loaded, missing and watched; pictures, water
and progress; landscapes and their tables; parts wearing table materials;
the levels file made, kept, made again when stale and remade on asking, as
many levels as the parts file's count) must be in `game/tests/models.c`'s
header comment (51 lines, cap 60); add only what it lacks, staying within
the cap.

Comments and index only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `game/tests/tests.md`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines game/tests/models.c`
prints 60 or less and no "over cap".
