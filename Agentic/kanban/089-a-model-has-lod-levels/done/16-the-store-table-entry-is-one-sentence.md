# 16 — The store table's index entry is one sentence
folder: 3d/src
after: none
decisions: 0168

## Change
`3d/src/src.md`: the entry for `models_store.h` is 325 characters, cap 300.
Cut it to one sentence under 260 characters naming what the file is: the
store's internal table and the helpers the store's files share. The list it
drops (landscape grid, held ids, heights and paint textures, pyramid and
dirts, material write mark, terrain lookup, `models_levels.c`'s calls) must
be in `3d/src/models_store.h`'s header comment; add there any item it does
not already say, keeping that header within its cap. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `3d/src/`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/src/models_store.h`
prints no "over cap".
