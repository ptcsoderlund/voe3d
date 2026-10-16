# 91 — The draw system's index entry fits its cap
folder: 3d/src
after: none
decisions: 0168

## Change
`3d/src/src.md`: the entry for `draw_system.c` is 301 characters, cap 300.
Cut it to one sentence under 260 characters naming what the file is: the
camera's view, the first light, sky, Haze and picture rows, and each
drawable's draw into the opened frame. Any detail it drops (an off camera
framing from the origin, each light's disc) must be in
`3d/src/draw_system.c`'s header comment; add there any item it does not
already say, keeping that header within its cap. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `3d/src/`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/src/draw_system.c`
prints no "over cap".
