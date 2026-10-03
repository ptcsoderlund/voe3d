# 25 — The probe volume is placed by the eye alone, so turning never moves it
folder: 3d
after: none
decisions: 0168, 0326, 0328, 0329

## Change
0329 point 1, for bug 01 of 051: the volume stood 24 m ahead of the eye, so a turn moved it, emptied
probes and faded the room. `voe_3d_bounce_grid_fit` is the one owner; its only callers are in 3d.
- `3d/include/3d/bounce_grid.h`: `voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_math_double3 eye)`,
  the view parameter gone. `VOE_3D_BOUNCE_AHEAD` goes; `VOE_3D_BOUNCE_BELOW` 8, the volume's cells
  below the eye's cell. Header rewritten: the lowest cell is the eye's cell (floor in double at
  `VOE_RENDER_BOUNCE_SPACING`) less 12 along x and z and less `VOE_3D_BOUNCE_BELOW` along y; why the
  eye and not the view (0328, 0329); why 8 below (a camera 10.9 m up keeps the ground out of the
  faded outer cell, a standing eye keeps a ceiling); `corner` as now.
- `3d/src/bounce_grid.c`: to match; forward is no longer read.
- `3d/src/draw_bounce.c`: the fit called with the frame's eye alone.
- `3d/src/draw_bounce.h`: `voe_3d_draw_bounce`'s comment fits the volume to the eye, not the view.
- `3d/tests/bounce_grid.c`: the turn and the view go; claims: at eyes near the origin, at negative
  coordinates and 10 km out, the eye's cell is `cell` + (12, 8, 12) and `corner` is the lowest cell's
  lowest corner about the eye; 2 m along x moves it one cell; a move inside the eye's cell keeps it.
  Header to match.
- `3d/tests/bounce_scene.c`: a TURN case after SETTLING, standing for bug 01: settled, the camera
  turned 90 degrees about Y in place for a few probing frames, each true (no capture pass opens),
  then turned back: the lit-side, shadow-foot and five even-ground pixels each within 1/255 of their
  values before the turn. Run it before changing `bounce_grid.c`: it must fail (the old fit opens
  capture passes on the turn); say so in the commit message. Header paragraph for the case.
- Entries: `3d/include/3d/3d.md` (`bounce_grid.h`), `3d/src/src.md` (`bounce_grid.c`),
  `3d/tests/tests.md` (`bounce_grid.c`, `bounce_scene.c`): no "24 m ahead", no "forward".

## Done when
`ctest --test-dir build/debug -R "^3d/(bounce|bounce_grid|bounce_scene)$"` passes and
`! grep -rn "VOE_3D_BOUNCE_AHEAD" 3d` exits 0.
