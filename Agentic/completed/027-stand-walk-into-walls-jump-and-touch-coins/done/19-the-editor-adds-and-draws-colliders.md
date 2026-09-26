# 19 — The editor steps colliders, fits a new one and draws the selected one
folder: editor
decisions: 0168, 0175, 0253, 0177

## Change
Feature steps 2–4 and 13 in the editor. 0253 point 1: `physics` added to the `editor` row in
`cmake/voe.cmake` (comment names 0253) and to `editor/CMakeLists.txt`'s DEPENDS. Add component,
the Inspector, Remove, undo and save already work for any described type; this card adds only
what is the collider's own.

- `editor/src/world_step.h`, `world_step.c` — one call to `voe_game_world_step` (card 16), which
  runs the collider's and body's drains with the rest; the header says the order is `game`'s now.
  No move: the editor steps nothing (0254).
- `editor/src/entities.h`, `entities.c` — `voe_editor_entities_component_add` gives a collider
  added to an entity with a shape the row `voe_3d_shape_collider(kind)` (card 14) instead of the
  type's default. Header point: a new collider starts out fitting the shape.
- `editor/src/view_passes.h`, `view_passes.c` — while an entity with a collider is selected, each
  view's frame sets `collider` to it, with the outline's pixels; the device capacities count one
  more transient range and object and `VOE_3D_COLLIDER_MARKER_VERTICES`/`_INDICES`.
- `editor/src/src.md` — the three entries.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor` exits 0.
