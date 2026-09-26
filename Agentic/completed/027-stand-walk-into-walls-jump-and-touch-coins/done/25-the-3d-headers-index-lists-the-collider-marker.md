# 25 — The 3d headers' index lists the collider marker
folder: 3d/include/3d
decisions: 0168

## Change
`checks.sh --all` finds `3d/include/3d/3d.md` does not list `collider_marker.h`, a header
card 14 added. Only the index changes; no header, no code.

- `3d/include/3d/3d.md` — a `collider_marker.h` entry, after `camera_marker.h`: a collider
  drawn in an editor's view as line quads about the eye (box edges, sphere circles, capsule
  outline), built in an arena from physics's world shape. Take the points from the header
  comment of `3d/include/3d/collider_marker.h`; read nothing else.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/include/3d` prints `FINDINGS: 0`.
2. `grep -c collider_marker.h 3d/include/3d/3d.md` prints 1.
