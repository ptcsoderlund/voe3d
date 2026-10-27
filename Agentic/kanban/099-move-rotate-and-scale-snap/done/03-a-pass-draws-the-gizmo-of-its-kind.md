# 03 — A pass draws the gizmo of its kind
folder: 3d
after: 01, 02
decisions: 0168, 0482, 0205, 0274

## Change
0482 point 3: one enum names which gizmo a pass draws, and the scale gizmo is drawn. This
changes `voe_3d_gizmoed`'s surface; the editor's one use (`.rings` in
`editor/src/view_passes.c`) is card 04's, so the editor does not build between the two.

- `3d/include/3d/gizmo.h` — `typedef enum { VOE_3D_GIZMO_ARROWS = 0, VOE_3D_GIZMO_RINGS,
  VOE_3D_GIZMO_SCALES } voe_3d_gizmo_kind;` with a comment: zeroed is the arrows, the mode is
  the caller's.
- `3d/include/3d/draw_system.h` — in `voe_3d_gizmoed`, `bool rings` becomes
  `voe_3d_gizmo_kind kind`; the type's paragraph on where the quads come from names the scales'
  quads too and sizes the transient pool from the largest of the three `_VERTICES` and of the
  three `_INDICES`; `marked`'s comment names `voe_3d_gizmo_scales_hit`. Include
  `3d/gizmo_scales.h`.
- `3d/src/draw_marks.c` — `voe_3d_draw_marks_gizmo` builds the arrows, rings or scales by
  `kind`, the scales with the world transform's rotation as `turn`; same two draws.
- `3d/src/draw_marks.h` — "arrows or rings" becomes "arrows, rings or scales".
- `3d/tests/draw_gizmo.c` — `.rings = true` becomes `.kind = VOE_3D_GIZMO_RINGS`, and a third
  case draws the scales the way the rings' case does and claims what it claims; its header
  follows.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — `gizmo.h`, `draw_system.h`, `draw_marks`
  and `draw_gizmo.c` entries name the kind and the scales.

## Done when
`ctest --test-dir build/debug -R '^3d/draw_gizmo$'` passes (it may skip with no GPU), and
`grep -rn "\.rings\b" 3d` prints nothing.
