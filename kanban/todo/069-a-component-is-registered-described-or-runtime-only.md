# 069 — a component is registered described, or runtime-only

claimed-by: -
blocked-by: -
decision: *A component is saved when it is described, and runtime-only is said out loud* (ADR-0150) — every point, including its table of the seven types.

## Goal

`voe_ecs_component_register` no longer accepts a NULL description. Every type is registered
either with its description or with `voe_ecs_runtime_only`. Camera and light gain descriptions;
mesh, material and panel are declared runtime-only. Nothing is saved yet — there is no writer —
so the observable change is the inspector and the compiler.

## Scope

**1. `ecs/include/ecs/component.h`, `ecs/src/component.c`.**
- `extern const voe_base_struct_description voe_ecs_runtime_only;` — a marker, compared by
  address. Its contents are an empty description; nothing reads them.
- `voe_ecs_component_register`'s description parameter is **declared non-null** with the C23
  attribute spelling this repository already uses, so a literal `NULL` fails to compile under
  `-Werror`. It also **asserts** the pointer is not NULL.
- `voe_ecs_component_description` returns **NULL for a runtime-only type**, as it does today for
  an undescribed one, so `editor/src/inspector.c` keeps working without an edit.
- New: `[[nodiscard]] bool voe_ecs_component_runtime_only(const voe_ecs_world *world, voe_ecs_type type);`
- Rewrite the header's sentence *`description` may be NULL, and is for an undescribed component*:
  a type is described or runtime-only; a described type's fields are what a scene saves
  (ADR-0149, ADR-0150); runtime-only is for state that means nothing in a file — a GPU id, a
  frame's range; and why NULL is refused — a forgotten description would otherwise be a quiet
  loss at every save.

**2. The five registrations passing NULL today.**

| File | Change |
|---|---|
| `3d/src/mesh_component.c` | `&voe_ecs_runtime_only` |
| `3d/src/material_component.c` | `&voe_ecs_runtime_only` |
| `3d/src/panel_component.c` | `&voe_ecs_runtime_only` |
| `scene/src/camera_system.c` | its description |
| `scene/src/light_system.c` | its description |

**3. Camera and light descriptions** — written exactly the way `transform_component.h` and
`identity_component.h` write theirs: a `VOE_SCENE_CAMERA_FIELDS(F, F_READ_ONLY)` list beside the
struct in `scene/include/scene/camera_component.h` and `VOE_BASE_DESCRIBE_STRUCT` after it, and
the same for `voe_scene_light` in `light_component.h`. Every field, in declaration order, with
the kinds `base/describe.h` has — `eye` FLOAT3; `yaw`, `pitch`, `fov_y`, `near_plane`,
`far_plane` FLOAT32; `direction`, `colour` FLOAT3; `intensity` FLOAT32. **Check the field order
against the structs as they are in the tree** and follow the struct, not this list, if they
differ. No field is read-only. Registration passes the description the way
`transform_system.c` does.

**4. Tests.**
- `ecs/tests/component.c`: every NULL becomes `&voe_ecs_runtime_only` or a test description;
  add a test that a runtime-only type reports `runtime_only` true and description NULL, and a
  described type reports false and its description.
- `scene/tests/`: camera and light descriptions are checked the way the transform's is — every
  field's offset and size against the struct. Where `scene/tests/transform.c` or `identity.c`
  proves both answers of the `VOE_BASE_DESCRIPTIONS` switch (ADR-0145 point 5), do the same for
  these two.

**5. Pages.** `ecs/ecs.md`, `scene/scene.md`, `3d/3d.md` — a line each where registration or the
components are described.

## What must not change

- The inspector's code. With camera and light described, the editor shows them expanded **only
  if the editor's world registers them** — it does not today, so the editor looks the same.
- Camera and light gain **no replace intent**; they are shown, not edited (ADR-0150
  consequences).
- Identity and transform registrations.
- No writer, no reader, no scene file. No folder but `ecs`, `scene`, `3d`.
- `voe_ecs_component_description`'s signature and its answer for described types.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest` passes.
- In a scratch file never committed, a `voe_ecs_component_register(..., NULL)` call fails to
  compile. Paste the diagnostic into Notes.
- `grep -rn 'component_register' --include=*.c . | grep -n 'NULL'` returns nothing outside
  `build/`.
- `voe_dev` and `voe_editor` run and look as they did.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

No component in the tree is registered without saying whether it is authored data, and a new
one that forgets cannot compile.

## Notes
