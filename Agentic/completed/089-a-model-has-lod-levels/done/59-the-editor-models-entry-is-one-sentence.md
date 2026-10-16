# 59 — The editor's model store entry is one sentence
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the entry for `models.c` is 310 characters, cap 300,
and wraps unevenly over three lines. Cut it to one sentence under 270
characters: the editor's one model store, made, filled, watched and re-read
through game/models.h, and its landscapes and materials edited and written
back. Details that may be dropped (emptied on a new folder; a failure's
notice; a prepared model reloaded; parts given; materials reloaded when a
read changed them; destroyed) are already in `editor/src/models.c`'s header
comment (11 lines); add only what it lacks.

Comments and index only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `editor/src/src.md`.
