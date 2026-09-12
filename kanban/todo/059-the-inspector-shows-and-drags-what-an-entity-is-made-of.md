# 059 — The inspector shows what the selected entity is made of, and drags its values

claimed-by: -
blocked-by: 056, 057, 058b, 060
status: todo
decision: *The world lists its component types, and a description travels with a registration* (ADR-0132) — walk the types, ask each for the entity, expand what is described; *An edit is a replace intent* (ADR-0134) point 4 — read, copy, change the bytes, submit, never `voe_ecs_component_set`; *The inspector edits by dragging first* (ADR-0136) point 4 — which kind gets which control; *A field can be marked read-only* (ADR-0139) point 2 — a read-only field is a label.

## Goal

With an entity selected, the `Inspector` panel lists every component it has by key name, and
for each described one a row per field, the editable ones dragged through the component's
replace intent.

## Scope

**1. `editor/src/inspector.h`, `editor/src/inspector.c`**, called from
`voe_editor_panel_draw` for `VOE_EDITOR_PANEL_INSPECTOR`, replacing the `Selected:` line card
058b put there. **The dock tree and its walk are not edited** (ADR-0142 point 2 — a panel's
contents are ordinary C).

- **The walk**: for every index below `voe_ecs_component_type_count`, the type at it, and
  `voe_ecs_component_get` for the selected entity; skip a NULL. **No component type is named
  anywhere in this file.**
- For each type present, a panel keyed by the name `component` and the type's index, so every
  key beneath it is unique, headed with `voe_ecs_component_key(...)->name`. A NULL
  `voe_ecs_component_description` → the heading and nothing else.
- Otherwise one row per field — its name, then:
  - **a field whose description is marked read-only → a label, whatever its kind;**
  - otherwise, by kind:

| Kind | Control | Per millimetre |
|---|---|---|
| `FLOAT32`, `FLOAT64` | a number box | 0.01 |
| `INT8`…`INT64`, `UINT8`…`UINT64` | a number box, its result rounded | 0.5 |
| `FLOAT2`, `FLOAT3`, `FLOAT4` | one number box per component, in a row | 0.01 |
| `BOOL` | a button labelled `true` or `false`; fired flips it | — |
| `QUAT` | three rows, `x`, `y` and `z`, each a number box showing an angle in degrees | 1.0 |
| `ENUM`, `CHAR`, `ENTITY`, `FLOAT4X4` | a label | — |

- A label shows: numbers and vectors as numbers; `BOOL` as `true` or `false`; `CHAR` its bytes
  up to the first zero or its count; `ENUM` its integer; `ENTITY` `<index>v<generation>`;
  `FLOAT4X4` the word `matrix`.
- Every label's text is formatted into the frame's arena, which outlives `voe_ui_frame_end`
  (see `voe_ui_label`).
- **A rotation row**: the three angles shown are a Z-Y-X decomposition of the quaternion,
  for display only. When a row's number box reports `changed`, the edit is
  `voe_math_quat_mul(voe_math_quat_from_axis_angle(axis, radians(new − shown)), q)` about the
  world's X, Y or Z axis. No angle is stored anywhere.

**2. The edit**, when a control changes a field:

- `voe_ecs_component_replace(world, type)`. **Not `set` → that component's fields are
  labels, not controls.**
- A zeroed buffer of `EDITOR_INTENT_BYTES` (256), asserted to be at least `value_size`; the
  entity at offset zero; the row from `voe_ecs_component_get` copied at `row_offset`
  (`row_size` bytes); the field's bytes at `row_offset + field.offset` overwritten; then
  `voe_ecs_intent_submit` with `replace.intent`.
- A full queue prints one line on stderr and that edit is dropped.

**3. `editor/editor.md`** — the two new files.

## What must not change

- **The editor never calls `voe_ecs_component_set`**, and `inspector.c` includes no
  component's header or system header.
- No typing, no text field and no click-to-type; a click on a number box does nothing.
- No folder but `editor/` is edited. A gap in `base`, `ecs`, `scene` or `ui` is
  `BLOCKED: <folder>, <why>`.

## Verify

- Linux: `cmake -P check.cmake` green.
- `cmake --preset editor`; build and run `voe_editor`. Select `Cube_2`: the identity shows
  `id` and `name` as labels; the transform shows position, rotation and scale as number
  boxes. Drag position x: it moves, and Shift slows it. Drag a rotation row: the angles
  follow and no `warning:` or `error:` line appears. Report what was checked by eye.
- Built with `--preset debug`: the same selection lists the two component names and expands
  neither.
- `grep -n 'voe_ecs_component_set\|scene/' editor/src/inspector.c` returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

Select a cube, drag its position with the mouse, and watch the number change — from an
inspector whose code has never heard the word *transform*, next to an id it cannot touch.

## Notes
