# 048 — A struct describes its fields, and `scene`'s transform is the first one

claimed-by: -
blocked-by: -
status: todo
decision: *A struct describes its fields, and the description lives in `base`* (ADR-0127) — a description is a name, a kind, an offset, a size and a count; the kind is an enum so `base` depends on nothing new; the declaring folder supplies the C type; the struct is always generated and the table only when asked. The vocabulary of kinds is ADR-0123. Why a struct and its description are written once together is ADR-0122.

## Goal

`base/include/base/describe.h` exists. `voe_scene_transform` is declared through it, its
layout unchanged. A test proves every described offset is the compiler's own and that the
declared kind matches the declared type.

## Scope

**1. `base/include/base/describe.h` — the type and the macros.**

- An enum of field kinds covering ADR-0123's vocabulary: the sized integer and
  floating-point types, `BOOL`, `FLOAT2` / `FLOAT3` / `FLOAT4`, `QUAT`, `FLOAT4X4`, `ENUM`,
  `CHAR` (for fixed-size name arrays) and `ENTITY`. Spell them `VOE_BASE_FIELD_*`.
- A field record: name, kind, offset, size, and element count (1 for a scalar, N for a
  fixed-size array).
- A struct record: the struct's name, a pointer to its fields, and how many.
- A declaration macro taking a struct name and a field-list macro. The field-list macro
  takes one parameter `F` and invokes it once per field as **`F(type, name, KIND)`** — the
  declaring folder supplies the C type, so nothing here maps a kind onto a type and `base`
  includes neither `math` nor `ecs`.
- The macro expands to **the `typedef struct` always**, and to the static field table and
  an accessor returning the struct record **only when descriptions are compiled in**.
- **A compile-time size check per field.** `base` knows each kind's expected size in bytes;
  assert it against `sizeof` of the declared type with `static_assert`, so a field declared
  `FLOAT3` whose type is a `float2` fails the build. `ENUM`, `CHAR` and `ENTITY` are the
  three where the check is against the element size rather than the whole field.
- The switch that includes or excludes the tables: one macro, off by default in a build
  that has not asked for it, and its name and meaning documented in the header.

**2. `scene/include/scene/transform_component.h` — declare the transform through it.**

The struct today is `voe_math_float3 position; voe_math_quat rotation; voe_math_float3
scale;`. Rewrite it as the field list plus the declaration macro. **The struct's layout,
field names, field order and public API do not change** — `voe_scene_transform_matrix` and
`voe_scene_transform_key` are untouched.

**3. Tests.**

- `base/tests/describe.c` — a struct declared locally in the test, covering a scalar, a
  vector, a fixed-size `char` array and an entity-kind field. Check names, kinds, counts,
  and that every offset equals `offsetof` for that field. Needs no graphics card and no
  window system.
- `scene/tests/transform.c` — add to the existing file: the transform's description has
  three fields named and kinded as declared, and each offset equals `offsetof`.

## What must not change

- **No component other than the transform is described.** Not the camera, not the light,
  not a `ui` panel. This card proves the mechanism on one.
- **`base` gains no dependency.** It must not include a `math` or an `ecs` header. If
  something appears to require one, that is a `BLOCKED:` and not a workaround.
- **The module map is untouched** — ADR-0022's DAG stands exactly as it is.
- **No identity component, no editor, no serializer, no inspector.** Those are later cards.
- `voe_scene_transform`'s size, layout and behaviour are identical before and after, with
  descriptions compiled in or out.
- `base/base.md`'s opening sentence currently reads *"Memory, containers, strings and
  assert"*. It needs widening for this file — that is in scope and is one line.

## Verify

- Linux: `cmake -P check.cmake` green.
- Both new tests pass under `ctest`.
- Build once with descriptions off and once with them on: `ctest` green both ways, and the
  engine's behaviour identical.
- `grep -rn 'math/\|ecs/' base/include base/src` returns nothing.
- From the planning root, `sh tools/hot.sh` reports no new `OVER`.

## Done looks like

A transform component that reads as a list of its fields, a table beside it that says where
each one lives, and a test that would fail if either drifted from the other. Nothing else in
the engine behaves differently, and a build that does not ask for descriptions does not
contain them.

## Notes
