# 23 — A collider's kind is a named number, as a shape's is
folder: physics
decisions: 0168, 0198, 0253

## Change
`voe_physics_collider.kind` is described as ENUM, which `authoring` neither writes nor reads, so a
scene holding a collider cannot be saved or loaded. 0198 says a named field stays a UINT32 (as
`3d/include/3d/shape_component.h` does for a shape's kind): the scene then carries `kind = 1`, the
cook writes the integer, and the editor shows the names as a dropdown. No other folder changes.

- `physics/include/physics/collider_component.h` — in `VOE_PHYSICS_COLLIDER_FIELDS`, `kind`
  becomes `F(uint32_t, kind, UINT32)`. The comment above the field list: kind is a UINT32 named
  by `voe_physics_collider_kind_names` (0198), written to a scene as its number; drop the line
  about the plain enum ENUM describes.
- `physics/tests/collider.c` — in `registration_tells_a_tool`: the description from
  `voe_physics_collider_description()` has a `kind` field of kind `VOE_BASE_FIELD_UINT32`, and
  `voe_base_names_find(description, "kind")` is non-NULL with `value_count` 4. The file's header
  gains the point that a named kind is a UINT32 so a scene can hold it.
- `physics/physics.md` — the `collider_component.h` entry: kind is a named number.
- `physics/tests/tests.md` — the `collider.c` entry: the kind field's description too.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder physics` prints `FINDINGS: 0`
   (builds `physics` and runs `physics/collider` among its tests).
2. `grep -rn "FIELD_ENUM" physics` prints nothing.
