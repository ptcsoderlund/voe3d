# 15 — The render tests index entries for the point-light tests fit their cap
folder: render/tests
after: none
decisions: 0168

## Change
In `render/tests/tests.md`, the entries for `point_shadows.c` (464
characters) and `point_lights.c` (351) are over the 300-character cap. Cut
each to one sentence that says what the file tests. Open the header comments
of `render/tests/point_shadows.c` and `render/tests/point_lights.c` and move
into each whatever its entry loses that the header does not already say.
Change no code and no other entry.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/tests`
reports no finding for `render/tests/tests.md`.
