# 42 — The game/models.h header fits the cap
folder: game/include/game
after: none
decisions: 0168

## Change
The whole suite found the header comment of `game/include/game/models.h` at
62 lines, cap 60. Comments only; no declaration changes (a public header:
other folders include it, so nothing in it but the comment moves).

Tighten the header to at most 60 lines without dropping a point. The LEVELS
paragraph is the longest and repeats detail (the `.parts` defaults, the swaps
written into a fresh file, the shipped game) that belongs to one function: the
why of that goes in a comment directly above `voe_game_models_update` in this
same header, leaving the paragraph its rule and decision numbers. Reflow
paragraphs whose last line is short. Measure with
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines game/include/game/models.h`.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING game/include/'`
prints nothing.
