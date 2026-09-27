# 19 — The 3d header index names the rings and the marker
folder: 3d/include/3d
decisions: 0168, 0274

## Change
Index only; no code. `checks.sh --all` finds `3d/include/3d/3d.md` missing two headers
that cards 05 and 04 added.

- `3d/include/3d/3d.md`: add an entry for `gizmo_rings.h` after `gizmo.h` (the rotate
  gizmo as arithmetic: three world-axis rings, the ring a ray meets, the angle a ray points
  at, its triangles) and one for `sun_marker.h` after `collider_marker.h` (a sun drawn as a
  circle and an arrow in line quads, and the ray that picks it). Take the points from each
  header's opening comment; keep each entry under 300 characters.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/include/3d` prints `FINDINGS: 0`.
