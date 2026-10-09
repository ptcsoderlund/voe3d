# 05 — A shape wears a material
folder: 3d
after: 04
decisions: 0168, 0399

## Change
0399 point 6 for shapes.

- `3d/include/3d/shape_component.h` — the shape row gains `material`, `F(char, material, CHAR,
  VOE_3D_MODEL_PATH)` (include `3d/model_component.h` for the constant, or define one equal),
  empty by default and in a file without it. Header point: an empty path, or one the store holds no
  loaded material for, draws the shape's colour as before; with one, the material with colour white.
- New internal `3d/src/draw_material.h` / `.c` — `const voe_3d_material
  *voe_3d_draw_material_named(const voe_3d_models *models, const char *path)`: the loaded material
  entry's part material for a non-empty path, else NULL. A linear find over the store, once per drawn
  entity per pass (0388: its part of the frame is the world pass's CPU).
- `3d/src/draw_group.c` — where a mesh's object record is built (its header says so): a shaped entity
  whose `material` names a loaded material draws with that shading and colour white.
- `3d/src/draw_bounce.c` and `3d/src/draw_shadows.c` — read their headers; where either builds a
  mesh's record itself rather than through `draw_group.c`, the same rule.

Update `3d/3d.md`'s `shape_component.h` line and `3d/src/src.md`.

New test `3d/tests/draw_material.c` (follow `3d/tests/draw_system.c` for a headless frame and its
read-back): a cube shape whose `material` names a loaded unlit material of colour red reads red at
the picture's centre; the same cube with an empty path reads its own grey; one naming a path not in
the store reads grey too.

## Done when
`ctest --test-dir build/debug -R '^3d/(draw_material|shape|draw_system)$'` and
`ctest --test-dir build/debug -R '^game/'` pass.
