# 45 — The shadow passes' header fits its cap
folder: 3d/src
after: none
decisions: 0168

## Change
`3d/src/draw_shadows.c`: its header comment is 62 lines against a cap of 60.
Shorten it to 60 or fewer without losing a fact: move the why that belongs to
one function out of the header to the comment above that function in the same
file. Candidates: the "who bounces" detail of the probe-bounce paragraph goes
above the function that decides whether anything bounces; the per-caster rules
(`cast_shadows`, `fade`, blocker masks) go above the function that tests a
caster. The header keeps what the file does, the order of the passes, why it
is its own call, and that the contract is in `3d/draw_system.h`. No code
changes; comments only. `3d/src/src.md` changes only if its entry for
`draw_shadows.c` names something that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/src/draw_shadows.c`
prints no "over cap".
