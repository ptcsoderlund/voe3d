# 16 — The 3d headers' index lists the camera marker
folder: 3d/include/3d
decisions: 0168, 0223

## Change
`3d/include/3d/3d.md` has no entry for `camera_marker.h`. Read the header comment of
`3d/include/3d/camera_marker.h` (its first paragraph is enough) and add one entry for it
to `3d/include/3d/3d.md`, beside `outline.h` and `pick.h`: a scene camera drawn in an
editor's view as box and frustum line quads, and the ray that picks the box. One
sentence, under 300 characters. No other file changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/include/3d` prints `FINDINGS: 0`.
