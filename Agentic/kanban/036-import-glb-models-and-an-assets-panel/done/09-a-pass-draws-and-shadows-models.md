# 09 — A pass draws and shadows models
folder: 3d
decisions: 0168, 0277, 0278, 0258, 0191

## Change
Needs card 07. Additive to `voe_3d_frame`: NULL draws no model, so no caller breaks
(0277 point 3).

- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `const voe_3d_models *models`, NULL
  none. `_frame` leaves it NULL (list it among the fields the caller sets). `_run`'s "AN ENTITY
  IS DRAWN WHEN" paragraph adds: a model row with a transform whose entry is loaded draws each
  part, world layer, white object colour, solid or blended by the part's material, `hidden`
  respected. `_shadows`' casters paragraph adds model parts under the same rule as meshes. The
  sizing paragraph counts parts.
- `3d/src/draw_system.c`: after the mesh walk, a walk over the model table doing for each part
  what the mesh walk does for a mesh (the object record from `voe_3d_draw_group_object_of` with
  no shape). Groups are sized for meshes, panels and every loaded part of every model row.
- `3d/src/draw_shadows.c`: the same walk in `draw_casters`: lit, not blended, world layer.
- `3d/include/3d/material_component.h`: the "EMISSION AND THE NORMAL MAP ARE STORED AND NOT
  READ" paragraph says both are shaded now (0278), in a phrase each.
- `3d/include/3d/mesh_component.h`: nothing unless a sentence says only meshes draw.
- `3d/tests/models.c`: cases, headless — a model entity before the camera with a store in the
  frame colours the centre pixel, without the store it does not; a hidden one does not.
- `3d/tests/shadows.c`: a model entity over the floor under a sun straight down darkens the
  floor beneath it, as the cube case does. Update both lines in `3d/tests/tests.md`, and the
  `draw_system.c` and `draw_shadows.c` lines in `3d/src/src.md`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0.
