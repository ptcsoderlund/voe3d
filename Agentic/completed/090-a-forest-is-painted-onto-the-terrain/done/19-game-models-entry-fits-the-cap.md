# 19 — The game models.h entry fits the cap
folder: game/include/game
after: none
decisions: 0168

## Change
The whole suite found index entries in `game/include/game/game.md` over the 300-character cap. Shorten these entries:
- `models.h` (325 characters), header `game/include/game/models.h`

Each entry becomes one sentence of at most 300 characters (the cap counts the whole `- `name` — ...` line). Any point the entry drops that the file's own header comment does not already make goes into that header, kept under its 60-line cap (measure with `checks.sh --header-lines <file>`). Comments and `.md` only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING game/include/game/'` prints nothing.
