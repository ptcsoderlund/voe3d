# 059 — The inspector shows what the selected entity is made of, and drags its values

claimed-by: claude-opus-5 (kanban-coder)
blocked-by: 056, 058b, 060
status: review
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

Verified on Linux (Fedora, Wayland, NVIDIA RTX 4070 Laptop, clang 22). Windows was
not checked — that is a bug report against this card's notes if it turns up, not a
reopening.

- `cmake -P check.cmake` exits zero: all 15 folders standalone, 42 tests, the
  analyser over 118 files, no warnings.
- Both presets built and run. `editor`: `Cube_2` shows `voe_scene_transform` with
  position and scale as three number boxes each and rotation as three rows `x`,
  `y`, `z` reading 0, 45 and 0 degrees — the eighth turn about +Y the scene is
  built with — and `voe_scene_identity` as two labels, `id 2` and `name Cube_2`,
  with no box behind either. `debug`: the same selection lists the two component
  names and expands neither.
- The drags were driven by a scripted pointer, reverted before this card moved,
  because there is no input-injection tool on this machine and a mouse-by-hand
  claim would have been unverifiable. Reading the component back every frame:
  13 mm past the dead zone on position x moved it 2.0000 → 2.1300, exactly
  0.01/mm; the same drag with `fine` moved it 2.1300 → 2.1430, a tenth of that;
  10 mm on the rotation `y` row took the quaternion from 45.0 to 55.0 degrees
  about +Y with its x and z components still exactly zero. Nothing else in the row
  moved in any of the three, and stderr stayed empty — the transform system settled
  nothing, so every quaternion submitted was still unit.
- `grep -n 'voe_ecs_component_set\|scene/' editor/src/inspector.c` returns nothing.
- `bash tools/hot.sh` at the planning root: no `OVER`; `editor/editor.md` 102/120.

Beyond the two files the Scope names, three small edits inside `editor/` that the
card implies but does not list. Say so rather than let them read as scope escape:

- `src/dock.c` — the `Selected:` line replaced, as the Scope asks. The tree and
  the walk are untouched.
- `src/scene.h` — one field, `voe_editor_inspector inspector`. A `ui` widget
  answers only after `voe_ui_frame_end` and `voe_editor_panel_draw` is handed the
  scene and nothing else, so the scene struct is the only place a panel can leave
  something to be asked about later. It is the same reason the Scene panel's rows
  are already in there.
- `src/interface.c` — two calls, `voe_editor_inspector_frame_begin` beside
  `voe_ui_frame_begin` and `_edits_read` beside `voe_editor_scene_clicks_read`.
  The frame's arena and the window between the frame's end and its rewind are both
  that function's and cannot be reached from inside the walk.

Two readings taken where the card is silent, both the narrow one:

- A component whose `voe_ecs_component_replace` reports `set` false has its fields
  drawn as labels. `ecs/component.h` says such a type is shown and not edited, and
  the alternative is a box that drags and changes nothing. Neither component in the
  editor's scene is one, so nothing on screen differs today.
- A field past `VOE_EDITOR_INSPECTOR_CONTROLS` is a label too, for the same reason.
  The interface's node budget refuses a frame long before 64 controls, so this is a
  floor under the read rather than a limit anybody meets.

Suggestions, not done because the card does not ask:

- Number boxes are natural width, so a row's boxes shift sideways as digits come
  and go. A fixed width would want a sizing parameter on `voe_ui_number_begin`,
  which is a card in `ui`.
- `editor` still has no `tests/`. The inspector's two testable pieces — the Z-Y-X
  decomposition and the bytes a control writes — are pure functions, but they are
  `static` in `inspector.c` and `editor` is an executable.
