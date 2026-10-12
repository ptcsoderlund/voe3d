# 22 — The draw system's index entry fits its cap
folder: 3d/src
after: none
decisions: 0168

## Change
`3d/src/src.md`: the `draw_system.c` entry is 312 characters, cap 300. Shorten it to one sentence
of at most 300 characters saying what the file does. Anything dropped that the header of
`3d/src/draw_system.c` does not already say moves into that header (read its header comment
only; the header stays at most 60 lines). No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/src` prints `FINDINGS: 0`.
