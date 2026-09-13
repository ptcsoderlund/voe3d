# 077 — `base`: a described field declares its shape

claimed-by: -
blocked-by: -
decision: *An array item is any value, and a described field declares its shape* (ADR-0154) points 5, 6, 9 and 10 — a field is one kind and up to seven fixed dimensions, and every kind may be an array.

## Goal

A field list can declare an array of any kind with up to seven dimensions, and its description
says the shape. No struct in the tree changes by a byte. Nothing reads the shape yet but the tests;
card 078 does.

## Scope

**1. `base/include/base/describe.h`.**

- `voe_base_field_description` gains, after `count`:

  ```c
  // How many dimensions: 0 for a single value, up to VOE_BASE_FIELD_RANK_MAX.
  uint32_t rank;
  // Outermost first, as C writes them; the entries past `rank` are 0.
  uint32_t dims[VOE_BASE_FIELD_RANK_MAX];
  ```

  with `#define VOE_BASE_FIELD_RANK_MAX 7`. **`count` keeps its meaning** — the total number of
  elements, the product of the dimensions, 1 for a single value — and so does `size`.
- **`F` and `F_READ_ONLY` take `(type, field, KIND, ...)`**: `type` is one element, then zero to
  seven dimensions. `F(voe_math_float3, path, FLOAT3, 4, 2)` declares `voe_math_float3 path[4][2];`
  and its row says rank 2, dims `{4, 2}`, count 8, size 96. `F(uint16_t, hp, UINT16)` is rank 0.
- **Every field's check is `sizeof(typeof(type)) == VOE_BASE_FIELD_SIZE_##KIND`** — exact, for every
  kind. The `VOE_BASE_FIELD_REPEATS_*` table and the modulo branch go.
- A dimension of 0, or more than seven, fails the build. Say in Notes how each failure reads.
- Turning `3, 2` into `[3][2]` and into `{3, 2}` is a counted dispatch up to seven; no recursion
  trick is needed or wanted. Keep the macros readable to the next person.
- **The header paragraphs.** Rewrite *THE DECLARING FOLDER SUPPLIES THE C TYPE…* and *THE BUILD
  REFUSES A KIND…* for the new spelling: the type is one element; the dimensions follow the kind,
  outermost first; every kind may be an array; for `CHAR` the innermost dimension is the string's
  bytes, so `CHAR, 32` is one string and `CHAR, 8, 32` eight of them; why the dimensions are
  spelled out (C cannot recover them from a type in a constant expression); ADR-0154.

**2. Declarations the new spelling breaks** — downstream, and the whole list today:
- `scene/include/scene/identity_component.h`: `F(char, name, CHAR, VOE_SCENE_IDENTITY_NAME)`.
- `base/tests/describe.c`: `F(char, label, CHAR, 13)`.
- `grep -rn 'F\(_READ_ONLY\)\?(.*\[' --include=*.h --include=*.c .` returns nothing afterwards.
  Anything it finds that is not a field line is left alone; say so in Notes.

**3. `base/tests/describe.c`.** Beside the existing thing, one more described struct and its
hand-written twin, compared by `sizeof` and every `offsetof` as the existing test does:
- `F(vector, grid, FLOAT3, 4, 2)` against `vector grid[4][2]` — rank 2, dims `{4, 2, 0, 0, 0, 0, 0}`,
  count 8, size 96.
- `F(char, names, CHAR, 8, 32)` — rank 2, count 256, size 256.
- `F(entity, links, ENTITY, 3)` — rank 1, count 3.
- `F(uint8_t, deep, UINT8, 1, 2, 1, 2, 1, 2, 1)` — rank 7, count 8.
- `F(int32_t, one, INT32)` — rank 0, dims all 0, count 1, size 4.
- A `F_READ_ONLY` field with dimensions carries both the shape and `read_only`.
- The existing thing's rows are unchanged except `label`'s new rank 1 and dims `{13}`.

## What must not change

- **Every struct's layout**, byte for byte. `voe_scene_identity` is still `uint64_t id; char name[64];`.
- `count` and `size` as every reader of a description uses them today: `ecs`, `editor`'s
  inspector and `authoring` compile and behave unchanged, and none of them is edited.
- `VOE_BASE_DESCRIPTIONS` off: no table, the same struct, the size checks still made.
- No reader of the shape anywhere but `base/tests`. No text format change.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R 'describe|identity'` passes.
- Before moving the card, declare `F(vector, bad, FLOAT2, 2)` and `F(int32_t, bad, INT32, 0)` in a
  scratch copy of the test, build, and paste both compiler messages into Notes; revert.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

`F(voe_math_float3, path, FLOAT3, 4, 2)` compiles to the struct C would have given, its row names
the shape, and every existing component is the same bytes it was.

## Notes
