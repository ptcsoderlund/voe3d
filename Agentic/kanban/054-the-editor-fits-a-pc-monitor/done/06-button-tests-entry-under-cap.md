# 06 — The `button.c` test index entry fits the cap
folder: ui/tests
after: none
decisions: 0168

## Change
Only `ui/tests/tests.md` and, if a point is lost, the header comment of
`ui/tests/button.c`. No code changes.

The `button.c` entry in `ui/tests/tests.md` is 366 characters; the cap is 300.
Shorten it to one sentence under 300 characters naming what the file pins
down: press/release orders, clipping decides what can be hit, a pointer-taking
container, held/selected drawn inverse, and the pad following the theme's
spacing. Drop the test name `button_pad_follows_spacing` and wording, not
points. The header of `button.c` must say the pad follows the theme's spacing
(card 01 added that case); add that point to the header if it is missing.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder ui/tests` prints no
finding naming `button.c`.
