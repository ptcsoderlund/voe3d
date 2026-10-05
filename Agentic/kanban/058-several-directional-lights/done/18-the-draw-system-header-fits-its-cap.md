# 18 — The draw system header fits its cap
folder: 3d/include/3d
after: none
decisions: 0168

## Change
`3d/include/3d/draw_system.h`: its header comment is 61 lines, cap 60. Shorten it to 60 or
fewer: tighten wording, or move the why of a single function to the comment above that function.
Comments only; no declaration changes.

## Done when
`checks.sh --folder 3d/include/3d` prints no FINDING naming `draw_system.h`.
