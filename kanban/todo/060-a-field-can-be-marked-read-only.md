# 060 — A field can be marked read-only in its description

claimed-by: -
blocked-by: -
status: todo
decision: *A field can be marked read-only, and an authored id is never replaced* (ADR-0139) points 1 and 2 — the declaring field list marks a field read-only, `base`'s field record carries the mark, the struct is unchanged, and a tool shows a marked field without editing it.

## Goal

A field list can say that a field is read-only, and `voe_base_field_description` says so
for that field. Nothing about the struct it declares changes.

## Scope

**1. `base/include/base/describe.h`.**

- **A field list takes two parameters, `F` and `F_READ_ONLY`,** each invoked with the same
  three arguments as today: `(type, name, KIND)`. A field listed through `F_READ_ONLY` is
  read-only; one listed through `F` is not.

```c
#define THING_FIELDS(F, F_READ_ONLY)   \
	F_READ_ONLY(uint64_t, id, UINT64) \
	F(voe_math_float3, position, FLOAT3)
```

- `voe_base_field_description` gains `bool read_only`.
- `VOE_BASE_DESCRIBE_STRUCT` hands the list the same member expansion for both parameters and
  the same size check for both, so **the struct and its checks are identical whichever a
  field is listed through**. Only the table's row differs: `.read_only = true` through
  `F_READ_ONLY`.
- With descriptions off, nothing new is emitted.
- The header's example and its paragraphs are updated to the two-parameter form, with one
  paragraph saying what read-only means — a tool shows the field and does not edit it; the
  struct and the program are unaffected; the mark is the declaring folder's to write, beside
  the field.

**2. Call sites this breaks — both updated here (ADR-0113).**

- `base/tests/describe.c`, `THING_FIELDS`: the two-parameter form, with one of its fields
  listed through `F_READ_ONLY`.
- `scene/include/scene/transform_component.h`, `VOE_SCENE_TRANSFORM_FIELDS`: the
  two-parameter form. **None of its three fields is read-only.**

**3. Tests — `base/tests/describe.c`.** The read-only field's record says `read_only`, every
other field's does not; every offset still equals `offsetof`; the struct's `sizeof` is the
same as a struct written by hand with the same members.

## What must not change

- **The struct any field list declares**: its members, order, size and alignment.
  `voe_scene_transform` stays 40 bytes with offsets 0, 12 and 28, as card 048 measured.
- The kinds, their sizes, and the size checks.
- `ecs`, and every other folder but `base` and the one `scene` header named above.
- No other property of a field — no hidden, no range, no unit.

## Verify

- Linux: `cmake -P check.cmake` green.
- `ctest` passes with descriptions off (`--preset debug`) and on (a scratch tree with
  `CMAKE_C_FLAGS=-DVOE_BASE_DESCRIPTIONS=1`).
- `voe_scene_transform`'s size and offsets are unchanged in both builds; say in Notes how
  that was measured.
- `grep -rn '_FIELDS(F)' base scene` returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A field list where one line reads `F_READ_ONLY`, a description that says so, and a struct
byte-for-byte what it was.

## Notes
