# 069 — a component is registered described, or runtime-only

claimed-by: claude-opus-5 (coder)
blocked-by: -
decision: *A component is saved when it is described, and runtime-only is said out loud* (ADR-0150) — every point, including its table of the seven types, and its amendment of 2026-09-13 on a build without descriptions.

## Goal

`voe_ecs_component_register` no longer accepts a NULL description. Every type is registered
either with its description or with `voe_ecs_runtime_only` — and a described type, in a build
with descriptions off, with `voe_ecs_description_compiled_out`. Camera and light gain descriptions;
mesh, material and panel are declared runtime-only. Nothing is saved yet — there is no writer —
so the observable change is the inspector and the compiler.

## Scope

**1. `ecs/include/ecs/component.h`, `ecs/src/component.c`.**
- `extern const voe_base_struct_description voe_ecs_runtime_only;` — a marker, compared by
  address. Its contents are an empty description; nothing reads them.
- `extern const voe_base_struct_description voe_ecs_description_compiled_out;` — the second
  marker, the same shape: *described, in a build that compiled descriptions out* (ADR-0150
  amendment).
- `voe_ecs_component_register`'s description parameter is **declared non-null** with the C23
  attribute spelling this repository already uses, so a literal `NULL` fails to compile under
  `-Werror`. It also **asserts** the pointer is not NULL.
- `voe_ecs_component_description` returns **NULL for either marker**, as it does today for an
  undescribed one, so `editor/src/inspector.c` keeps working without an edit.
- New: `[[nodiscard]] bool voe_ecs_component_runtime_only(const voe_ecs_world *world, voe_ecs_type type);`
  — true for `voe_ecs_runtime_only` only; **false for `voe_ecs_description_compiled_out`**.
- Rewrite the header's sentence *`description` may be NULL, and is for an undescribed component*:
  a type is described or runtime-only; a described type's fields are what a scene saves
  (ADR-0149, ADR-0150); runtime-only is for state that means nothing in a file — a GPU id, a
  frame's range; why NULL is refused — a forgotten description would otherwise be a quiet
  loss at every save; and that a described type's descriptions-off branch passes
  `voe_ecs_description_compiled_out`, never NULL and never runtime-only.

**2. The five registrations passing NULL today.**

| File | Change |
|---|---|
| `3d/src/mesh_component.c` | `&voe_ecs_runtime_only` |
| `3d/src/material_component.c` | `&voe_ecs_runtime_only` |
| `3d/src/panel_component.c` | `&voe_ecs_runtime_only` |
| `scene/src/camera_system.c` | its description |
| `scene/src/light_system.c` | its description |

**And the two `#else` branches returning NULL today** — `transform_description()` in
`scene/src/transform_system.c` and `identity_description()` in `scene/src/identity_system.c` —
return `&voe_ecs_description_compiled_out`. That is the only change to those registrations.

**3. Camera and light descriptions** — written exactly the way `transform_component.h` and
`identity_component.h` write theirs: a `VOE_SCENE_CAMERA_FIELDS(F, F_READ_ONLY)` list beside the
struct in `scene/include/scene/camera_component.h` and `VOE_BASE_DESCRIBE_STRUCT` after it, and
the same for `voe_scene_light` in `light_component.h`. Every field, in declaration order, with
the kinds `base/describe.h` has — `eye` FLOAT3; `yaw`, `pitch`, `fov_y`, `near_plane`,
`far_plane` FLOAT32; `direction`, `colour` FLOAT3; `intensity` FLOAT32. **Check the field order
against the structs as they are in the tree** and follow the struct, not this list, if they
differ. No field is read-only. Registration passes the description the way
`transform_system.c` does, through a wrapper whose `#else` branch returns
`&voe_ecs_description_compiled_out`.

**4. Tests.**
- `ecs/tests/component.c`: every NULL becomes `&voe_ecs_runtime_only` or a test description;
  add a test that a runtime-only type reports `runtime_only` true and description NULL, a
  described type reports false and its description, and a type registered with
  `voe_ecs_description_compiled_out` reports false and description NULL.
- `scene/tests/`: camera and light descriptions are checked the way the transform's is — every
  field's offset and size against the struct. Where `scene/tests/transform.c` or `identity.c`
  proves both answers of the `VOE_BASE_DESCRIPTIONS` switch (ADR-0145 point 5), do the same for
  these two — `scene/tests/camera.c` and `light.c` exist and gain the same `BUILD_DESCRIBES`
  switch. **In all four — `transform.c`, `identity.c`, `camera.c`, `light.c` — the
  `!BUILD_DESCRIBES` half also checks the registered type is not runtime-only.**

**5. Pages.** `ecs/ecs.md`, `scene/scene.md`, `3d/3d.md` — a line each where registration or the
components are described.

## What must not change

- The inspector's code. With camera and light described, the editor shows them expanded **only
  if the editor's world registers them** — it does not today, so the editor looks the same.
- Camera and light gain **no replace intent**; they are shown, not edited (ADR-0150
  consequences).
- Identity and transform registrations, apart from their `#else` branch (section 2).
- No writer, no reader, no scene file. No folder but `ecs`, `scene`, `3d`.
- `voe_ecs_component_description`'s signature and its answer for described types.

## Verify

- Linux: `cmake -P check.cmake` green, **step 6c *descriptions off* included**; `ctest` passes.
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

**Verified on Linux** (Fedora 44, clang 22, RTX 4070). Windows not checked.

- `cmake -P check.cmake` green: every step, including **6c descriptions off
  (scene, 4 passed)** and the analyser (124 files). ctest 45 passed; ecs 3, scene 4.
- The grep for `component_register … NULL` returns nothing.
- `voe_dev` and `voe_editor` both ran for about 5 s and drew normally, with no
  assert. The editor printed `descriptions  compiled in` and showed Scene with
  Cube, Cube_2, Marker and "Nothing selected".
- `tools/hot.sh`: all hot files under their ceilings.
- A scratch `voe_ecs_component_register(world, &key, 4, 1, NULL)` (never
  committed) fails to compile:

      null_register.c:7:54: error: null passed to a callee that requires a non-null argument [-Werror,-Wnonnull]
          7 |         (void)voe_ecs_component_register(world, &key, 4, 1, NULL);
            |                                                             ^~~~

**For the reviewer: the NULL assert exists only at -O0.** The attribute is
`[[gnu::nonnull(5)]]`, the spelling `[[gnu::format]]` already set. Because of it,
clang deletes `description != NULL` from -O1 up. I compiled `component.c` at
-O0, -O1 and -O2 and looked for the assert's message: it is there at -O0 and gone
at -O1. So the assert guards Debug builds, which check.cmake and daily work use.
In Release, a NULL that reaches the call through a variable is not caught. A
literal NULL fails to compile in every build. This is written at the assert.
Nothing else in the tree works around it, and working around it would need a
flag in `cmake/voe.cmake`, outside this card.

**How the pieces were done.** The two markers are `= { 0 }` objects compared by
address. `voe_ecs_component_description` returns NULL for both, so `inspector.c`
is unchanged. Camera and light register through `camera_description()` and
`light_description()`, written like transform's. In all four scene tests the
description walk now checks the type is not runtime-only in both builds. That
covers the `!BUILD_DESCRIBES` half the card asks for. Camera's and light's
`check_field` also check each field's size and that it is not read-only. Field
order follows the structs as they were, which matches the card's list. The
per-field comments that sat inside the two structs now sit above their field
lists, because a `//` comment cannot go inside a line-continued macro.

No `DEVIATION:` or `BLOCKED:` markers.

