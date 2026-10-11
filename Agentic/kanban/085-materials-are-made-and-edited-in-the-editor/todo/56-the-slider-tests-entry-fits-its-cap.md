# 56 — The slider tests' entry fits its cap
folder: ui/tests
after: none
decisions: 0168

## Change
`ui/tests/tests.md`: the `slider.c` entry is 390 characters, cap 300. Cut it
to one sentence naming what the file pins: the thumb's place across the
range, a drag moving the value, and a shown slider that takes no input. The
detail it drops (the dead zone, the untouched frame handing the value back,
the clamped thumb) is already in `ui/tests/slider.c`'s header comment; read
that header and add to it only what it lacks. Change no code.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder ui/tests` prints no
FINDING line naming `ui/tests/tests.md`.
