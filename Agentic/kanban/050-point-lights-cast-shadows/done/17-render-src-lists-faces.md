# 17 — The render src index lists point_shadow_faces.c on its own
folder: render/src
after: none
decisions: 0168

## Change
`render/src/src.md` has one combined entry "`point_shadow_faces.h`, `.c`",
so the check does not see `point_shadow_faces.c` listed. Split it into two
entries as `bounce_schedule.h` / `bounce_schedule.c` are: the `.h` entry
says what the calls compute, the `.c` entry what lives in it. Read the
header comments of `render/src/point_shadow_faces.h` and `.c` for the
content. Each entry one sentence, under 300 characters. Change no code.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/src`
reports no finding for `render/src/src.md`.
