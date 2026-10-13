# 45 — The render source's draw.c entry is one sentence
folder: render/src
after: none
decisions: 0168

## Change
In `render/src/src.md` the entry for `draw.c` is 330 characters; the cap
is 300. Shorten it to one sentence under 300 characters: the draws inside a
pass, one object record per mesh draw, the point-shadow pass's per-face
instances, and the sides as a dynamic cull mode. Detail cut from the entry
(the depth clear and `plain` push, rebinds and cull mode recorded only on
change) stays said in the header comment of `render/src/draw.c`; add it
there only if missing, keeping that header within 60 lines (measure with
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines render/src/draw.c`). No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/src` prints no FINDING naming `render/src/src.md`.
