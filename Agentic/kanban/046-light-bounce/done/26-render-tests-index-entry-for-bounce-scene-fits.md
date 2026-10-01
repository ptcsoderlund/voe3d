# 26 — The render tests index entry for bounce_scene.c fits its cap
folder: render/tests
after: none
decisions: 0168

## Change
Documentation only; no test code changes.

`render/tests/tests.md`: the entry for `bounce_scene.c` is 376 characters,
the cap is 300. Cut it to one sentence well under 300 characters: the bounce
as the editor runs it, off the origin about the eye, holding through
scrolling and two views in one frame; headless.

`render/tests/bounce_scene.c`, header comment only: it already states each
claim the entry drops (the wall reddening nearby ground, never darkening,
twelve scrolled frames, two views matching each alone). Confirm; add only
what is missing.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `render/tests/tests.md`.
