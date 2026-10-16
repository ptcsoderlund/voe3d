# 58 — The 3d levels test entries are one sentence
folder: 3d/tests
after: none
decisions: 0168

## Change
`3d/tests/tests.md`: two entries are over the 300-character cap.

- `models_levels.c` (321 characters): cut to one sentence under 270
  characters: the store loading a sphere's levels file, its own `_LOD1`
  levels and a far swap, leaking none across reloads; skips without a card.
  Details that may be dropped (three levels or more, bounds non-increasing,
  the radius; NULL or another file's levels giving level 0 alone; the own
  level at bound 0.5; level 1's bound halved) are already in
  `3d/tests/models_levels.c`'s header comment (22 lines); add only what it
  lacks.
- `model_levels.c` (356 characters): cut to one sentence under 270
  characters: a sphere's levels file made and read back as asked, going
  stale on change, and a view's level and pixel share chosen by screen size;
  needs no graphics card. Details that may be dropped (the count, strength
  and swap asked; stale on a `.glb` or version change, versions 1 and 2; the
  share in the band below a bound) are already in `3d/tests/model_levels.c`'s
  header comment (29 lines); add only what it lacks.

Comments and index only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `3d/tests/tests.md`.
