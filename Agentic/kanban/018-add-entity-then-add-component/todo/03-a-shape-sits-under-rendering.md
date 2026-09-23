# 03 — A shape sits under Rendering in Add component
folder: 3d
decisions: 0168, 0217, 0221

## Change
- `3d/src/shape_component.c` — the registration sets `voe_ecs_component_menu_set(world, type, "Rendering /
  Shape")` beside its `voe_ecs_component_needs_set` (`ecs/include/ecs/component.h`).
- `3d/include/3d/shape_component.h` — one point: where a shape sits in Add component and that the path is
  registered with the type (0221).
- `3d/3d.md` — the `shape_component.h` entry gains the phrase.
- `3d/tests/shape.c` — in the test that checks the registration (the one checking the needed type), one check
  that `voe_ecs_component_menu` of the shape type equals `"Rendering / Shape"` (strcmp).

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `3d` exits 0, with the new check in `3d/shape`.
