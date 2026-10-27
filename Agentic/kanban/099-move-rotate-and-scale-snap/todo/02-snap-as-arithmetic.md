# 02 — Snap as arithmetic
folder: 3d
after: 01
decisions: 0168, 0482, 0438

## Change
0482 point 4: the three roundings a snapping drag applies, pure and tested, so the editor only
calls them.

- `3d/include/3d/gizmo_snap.h` (new):
  - `voe_math_double3 voe_3d_gizmo_snap_position(voe_math_double3 position,
    voe_3d_gizmo_handle handle, double step)` — the world axes `handle` drags (X, Y, Z alone;
    XY, YZ, ZX their two) to the nearest multiple of `step`, the rest untouched;
  - `float voe_3d_gizmo_snap_angle(float angle, float step)` — `angle` wrapped to (−π, π], then
    to the nearest multiple of `step`; radians;
  - `voe_math_float3 voe_3d_gizmo_snap_scale(voe_math_float3 scale,
    voe_3d_gizmo_handle handle, float step)` — the component an axis handle scales, all three
    for CENTRE, to the nearest multiple of `step` and never below one step.
  A step at or below nought, and NONE where a handle is asked, assert (rule 13). Header points:
  the world grid and not steps from the press for positions (0438), why turns are whole steps
  from the press, why a scale never reaches nought, that the Inspector never calls this.
- `3d/src/gizmo_snap.c` (new) — the three, a position rounded in double.
- `3d/tests/gizmo_snap.c` (new) — 0.37 on X with step 0.5 lands 0.5 and Y, Z keep theirs; XY
  rounds x and y only; 1.26 at step 1 is 1; 100 km out keeps the step exact; 22° at 15° steps
  is 15°, a sweep just past π snaps as its wrapped angle; 0.04 scales to 0.1, 1.26 on X to 1.3
  with Y and Z kept, CENTRE rounds all three.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — entries for the new files.

## Done when
`ctest --test-dir build/debug -R '^3d/gizmo_snap$'` passes.
