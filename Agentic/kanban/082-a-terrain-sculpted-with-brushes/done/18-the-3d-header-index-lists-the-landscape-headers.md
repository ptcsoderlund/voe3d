# 18 — The 3d header index lists the landscape headers
folder: 3d/include/3d
after: none
decisions: 0168, 0379

## Change
`3d/include/3d/3d.md`: add one entry each for `brush_marker.h` and `landscape.h`,
in the list's style (one sentence, under 300 characters). Read the header comment
of `3d/include/3d/brush_marker.h` and `3d/include/3d/landscape.h` for what to say:
the brush's two rings on the ground as line quads about the eye; the arithmetic on a
landscape's height grid (height under a point, ray to the ground, brush stamp,
4 × 4 chunks). Place them near related entries (markers; `models.h`). No code change.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/include/3d` prints `FINDINGS: 0`.
