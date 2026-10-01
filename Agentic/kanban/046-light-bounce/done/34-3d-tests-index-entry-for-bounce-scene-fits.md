# 34 — The 3d tests index entry for bounce_scene.c fits its cap
folder: 3d/tests
after: none
decisions: 0168

## Change
Documentation only; no test code changes.

`3d/tests/tests.md`: the entry for `bounce_scene.c` is 331 characters, the
cap is 300. Cut it to one sentence well under 300 characters: bug 01 through
the editor's calls at sun 1 and π, the red box tinting nearby ground while
its own shadow stays faint and no ground darkens; skips without a graphics
card.

`3d/tests/bounce_scene.c`, header comment only: it already states each claim
the entry drops (the 12/255 tint 1 m from the lit face, the 8/255 bound on
the shadow side, no pixel darker than the no-bounce reference). Confirm; add
only what is missing.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `3d/tests/tests.md`.
