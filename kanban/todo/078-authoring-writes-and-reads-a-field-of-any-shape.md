# 078 — `authoring` writes and reads a field of any shape

claimed-by: -
blocked-by: 070, 071, 077
decision: *An array item is any value, and a described field declares its shape* (ADR-0154) points 1–4, 7 and 8, and its paragraph on what a writer emits — arrays nest as the field's shape says, to 8 levels.

## Goal

`authoring`'s writer and reader handle every described field as its shape says, however many
dimensions it has, and a nested array survives the round trip. The one-level array code of cards
070 and 071 is gone, and their hand-written test descriptions are declared through the macro.

## Scope

**1. The writer, `authoring/src/scene_write.c`.** Walk `field->rank` and `field->dims`:
- **Rank 0**: the element, as today. **Otherwise** one `[` … `]` per dimension, outermost first,
  `, ` between items, no blank inside the brackets; a leaf of a vector kind brings its own
  `[x, y, z]`.
- **`CHAR`**: the innermost dimension is one string. `CHAR, 32` writes `"…"` as today; `CHAR, 3, 8`
  writes `["a", "b", ""]`.
- **`ENTITY`**: each element maps to its authored id as today.
- Every refusal and warning that names a field also names the element's index, `tags[1]`.
- A loop or recursion over the description, either: the description is the program's own, not read
  from a file, so rule 14 does not bind. Say which in the function's comment.

**2. The reader, `authoring/src/scene_read.c`.** A value is read against the field's shape:
- **The grammar is ADR-0154 point 1**: any array item may be an array; blanks around items and
  brackets are tolerated.
- **An explicit stack and a named limit of 8 bracket levels** (rule 14), with a file-header comment
  saying why. Past 8 refuses the file, the message naming the limit — before any count is checked.
- **Every level has exactly its dimension's count**, and every leaf reads as the field's kind: a
  ragged `[[1], [1, 2]]` and a mixed `[1, "a"]` refuse, with the line.
- **A string inside an array** is `"…"` with the escapes `\"` and `\\` and nothing else, and `,` and
  `]` inside it are bytes. The sectioned parser does not check escapes inside an array; this does.
- **`ENTITY` elements** are held as authored ids for pass two, each one patched as 071 patches one.

**3. Headers and map.** `scene_write.h` and `scene_read.h`: the paragraph on values says arrays nest
as the field's shape, `CHAR`'s innermost dimension is the string, the reader's limit is 8, ADR-0154.
`authoring/authoring.md`: one line if it describes values.

**4. `authoring/tests/`.**
- **070's every-kind component and 071's array test component are declared through
  `VOE_BASE_DESCRIBE_STRUCT`**; the hand-written description tables go; their expected texts stand.
- One test-only described component for shapes, used by both tests:
  - `F(int32_t, pair, INT32, 2, 3)` holding 1–6 → `pair = [[1, 2, 3], [4, 5, 6]]`
  - `F(voe_math_float3, points, FLOAT3, 2)` → `[[0, 0, 0], [1, 2.5, -3]]`
  - `F(voe_math_float3, grid, FLOAT3, 2, 2)` → three levels of brackets
  - `F(char, tags, CHAR, 3, 8)` holding `a`, `x"y\z`, empty → `["a", "x\"y\\z", ""]`
  - `F(uint8_t, deep, UINT8, 1, 1, 1, 1, 1, 1, 2)` → `[[[[[[[0, 7]]]]]]]`
  - `F(voe_math_float3, deepest, FLOAT3, 1, 1, 1, 1, 1, 1, 1)` → eight `[` in a row
  - `F(voe_ecs_entity, links, ENTITY, 2)` naming an authored entity and a dead one → `[3, 0]`
- **Write**: that component's section byte for byte; a `tags` element holding a byte below `0x20`
  refuses and names `tags[n]`; **card 070's three-entity expected text is unchanged.**
- **Round trip**, both directions as 071 does them, with the shapes component on two entities and
  `links` naming each other: bytes identical text-first, rows identical world-first.
- **Read, each refusing and leaving zero entities**: `pair` as `[[1, 2, 3], [4, 5]]`, as
  `[1, 2, 3, 4, 5, 6]`, as `[[1, 2, 3], [4, 5, "6"]]`; `tags` as `["a", "C:\x", ""]`, and with an
  eight-byte string, one too long for its slot; `deepest` with a ninth level; `points` as `[]`.
- **Read, tolerated**: `pair = [ [1,2,3] ,[4, 5,6] ]` loads and writes back canonical.

## What must not change

- `base`, `scene`, `ecs`, `assets`. A gap in `describe.h`'s shape is a report.
- The text written for, and the reading of, every field of rank 0 and every single string.
- 071's two passes, its kept sections, its refusals and warnings; no file I/O, no editor call site.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R authoring` passes.
- `grep -rn 'voe_base_field_description.*=\s*{' authoring/tests`: every table it still finds has a
  comment saying why the macro cannot declare it.
- Paste the shapes component's written section into Notes.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A field of any shape is written as nested brackets matching its dimensions, read back to the same
bytes, refused when the text's shape is not the field's, and every scene 070 and 071 handled is
unchanged.

## Notes
