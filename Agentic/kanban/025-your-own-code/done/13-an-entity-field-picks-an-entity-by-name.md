# 13 — An entity field picks an authored entity by name
folder: editor
decisions: 0168, 0241, 0242

## Change
Point 9 of 0242, second half, and the feature's last card. Read the headers of
`editor/src/inspector.h`, `editor/src/inspector_edit.h`, `editor/src/inspector_place.h`,
`editor/src/inspector_value.h`, `editor/src/scene.h`, `editor/src/interface.h`,
`scene/identity_system.h` and `game/world.h`.

- `editor/src/entity_field.h`, `editor/src/entity_field.c` (new) — the choices and the words:
  `uint32_t voe_editor_entity_field_choices(const voe_ecs_world *, voe_ecs_entity *out, uint32_t
  room)`: a zeroed entity (None) first, then every authored entity ascending by id;
  `const char *voe_editor_entity_field_label(const voe_ecs_world *, voe_ecs_entity, arena)`: its
  identity's name, or "None" for a zeroed or dead one or one with no identity. The header: why
  names and not ids, and that only authored entities are offered (only they are saved).
- `editor/src/inspector.h` — `voe_editor_dropdown` gains `entities` (the list is entity_field's
  choices, `names` NULL); `voe_editor_dropdown_row` gains `entity`; `VOE_EDITOR_DROPDOWN_ROWS`
  becomes 1 + `VOE_GAME_WORLD_AUTHORED`, its comment saying why.
- `editor/src/inspector.c` — an editable ENTITY field is a button with its label that opens the
  dropdown in entity mode, as a named field's does; `dropdown_list` draws the choices by label in
  that mode, each row carrying its entity.
- `editor/src/inspector_value.c` — an ENTITY shown as a label is entity_field's label, not
  `<index>v<generation>`.
- `editor/src/inspector_edit.h`, `editor/src/inspector_edit.c` —
  `voe_editor_inspector_entity_submit(inspector, world, entity, type, offset, voe_ecs_entity
  target)`, as `_named_submit` is.
- `editor/src/inspector_buttons.c` — a fired row in entity mode submits through it.
- `editor/src/interface.h` — the NODES and ELEMENTS sums for the longer list, and the numbers.
- `editor/src/src.md` — entries for entity_field.h and entity_field.c.

## Done when
1. `checks.sh --all` prints `FINDINGS: 0`.
2. In `p=$(mktemp -d)` with `cp -r game/example/. $p`, `build/debug/editor/voe_editor --capture
   $p/shot.png $p 2>$p/err` exits 0 and `$p/err` is empty.
3. The human's: `feature.md`'s How to test, steps 1 to 11, on a copy of `game/example` (step 8
   and 10 edit its `Code/`; the repository's copy stays as it is). Step 11 on any project with no
   `Code/`.
