# 002 Fields of any shape — tasks

- [ ] 1. `base/` — a described field declares its shape
  - Change: Decision: `Agentic/decisions/0154-an-array-item-is-any-value-and-a-field-declares-its-shape.md`
    points 5, 6, 9 and 10 — a field is one kind and up to seven fixed dimensions, and every kind
    may be an array.

    Goal: a field list can declare an array of any kind with up to seven dimensions, and its
    description says the shape. No struct in the tree changes by a byte. Nothing reads the shape
    yet but the tests; task 2 of this feature does. Done looks like:
    `F(voe_math_float3, path, FLOAT3, 4, 2)` compiles to the struct C would have given, its row
    names the shape, and every existing component is the same bytes it was.

    **1. `base/include/base/describe.h`.**
    - `voe_base_field_description` gains, after `count`:

      ```c
      // How many dimensions: 0 for a single value, up to VOE_BASE_FIELD_RANK_MAX.
      uint32_t rank;
      // Outermost first, as C writes them; the entries past `rank` are 0.
      uint32_t dims[VOE_BASE_FIELD_RANK_MAX];
      ```

      with `#define VOE_BASE_FIELD_RANK_MAX 7`. **`count` keeps its meaning** — the total number
      of elements, the product of the dimensions, 1 for a single value — and so does `size`.
    - **`F` and `F_READ_ONLY` take `(type, field, KIND, ...)`**: `type` is one element, then zero
      to seven dimensions. `F(voe_math_float3, path, FLOAT3, 4, 2)` declares
      `voe_math_float3 path[4][2];` and its row says rank 2, dims `{4, 2}`, count 8, size 96.
      `F(uint16_t, hp, UINT16)` is rank 0.
    - **Every field's check is `sizeof(typeof(type)) == VOE_BASE_FIELD_SIZE_##KIND`** — exact, for
      every kind. The `VOE_BASE_FIELD_REPEATS_*` table and the modulo branch go.
    - A dimension of 0, or more than seven, fails the build. Say in your report how each failure
      reads.
    - Turning `3, 2` into `[3][2]` and into `{3, 2}` is a counted dispatch up to seven; no
      recursion trick is needed or wanted. Keep the macros readable to the next person.
    - **The header paragraphs.** Rewrite *THE DECLARING FOLDER SUPPLIES THE C TYPE…* and *THE BUILD
      REFUSES A KIND…* for the new spelling: the type is one element; the dimensions follow the
      kind, outermost first; every kind may be an array; for `CHAR` the innermost dimension is the
      string's bytes, so `CHAR, 32` is one string and `CHAR, 8, 32` eight of them; why the
      dimensions are spelled out (C cannot recover them from a type in a constant expression);
      ADR-0154. Also update the example in that header that spells `F(char[32], name, CHAR)`.

    **2. Declarations the new spelling breaks** — downstream, and the whole list today (edited under
    the project `CLAUDE.md`'s call-site rule; name them in your report):
    - `scene/include/scene/identity_component.h`: today `F(char[VOE_SCENE_IDENTITY_NAME], name, CHAR)`,
      becomes `F(char, name, CHAR, VOE_SCENE_IDENTITY_NAME)`.
    - `base/tests/describe.c`: today `F(char[13], label, CHAR)`, becomes `F(char, label, CHAR, 13)`.
    - `grep -rn 'F\(_READ_ONLY\)\?(.*\[' --include=*.h --include=*.c .` returns nothing afterwards.
      Anything it finds that is not a field line is left alone; say so in your report.

    **3. `base/tests/describe.c`.** Beside the existing described struct, one more described struct
    and its hand-written twin, compared by `sizeof` and every `offsetof` as the existing test does:
    - `F(vector, grid, FLOAT3, 4, 2)` against `vector grid[4][2]` — rank 2, dims
      `{4, 2, 0, 0, 0, 0, 0}`, count 8, size 96.
    - `F(char, names, CHAR, 8, 32)` — rank 2, count 256, size 256.
    - `F(entity, links, ENTITY, 3)` — rank 1, count 3.
    - `F(uint8_t, deep, UINT8, 1, 2, 1, 2, 1, 2, 1)` — rank 7, count 8.
    - `F(int32_t, one, INT32)` — rank 0, dims all 0, count 1, size 4.
    - A `F_READ_ONLY` field with dimensions carries both the shape and `read_only`.
    - The existing struct's rows are unchanged except `label`'s new rank 1 and dims `{13}`.

    **4. `base/base.md`.** Update the `describe.h` line only if its one sentence no longer holds.

    **Before finishing**, declare `F(vector, bad, FLOAT2, 2)` and `F(int32_t, bad, INT32, 0)` in a
    scratch copy of the test, build, include both compiler messages in your report; revert.

    **What must not change.**
    - **Every struct's layout**, byte for byte. `voe_scene_identity` is still
      `uint64_t id; char name[64];`.
    - `count` and `size` as every reader of a description uses them today: `ecs`, `editor`'s
      inspector and `authoring` compile and behave unchanged, and none of them is edited.
    - `VOE_BASE_DESCRIPTIONS` off: no table, the same struct, the size checks still made.
    - No reader of the shape anywhere but `base/tests`. No text format change.
  - Covers: 1, 2, 7 (layout half), 8
  - Depends on: -
  - Done when: `cmake -P check.cmake` exits 0 on Linux, and
    `ctest --test-dir build/check/root -R 'describe|identity'` passes.

- [ ] 2. `authoring/` — writes and reads a field of any shape
  - Change: Decision: `Agentic/decisions/0154-an-array-item-is-any-value-and-a-field-declares-its-shape.md`
    points 1–4, 7 and 8, and its paragraph on what a writer emits — arrays nest as the field's shape
    says, to 8 levels.

    Goal: `authoring`'s writer and reader handle every described field as its shape says, however
    many dimensions it has, and a nested array survives the round trip. The one-level array code of
    the existing writer and reader (history/cards/phase4_editor/070-authoring-exists-and-writes-a-scene.md,
    history/cards/phase4_editor/071-authoring-reads-a-scene.md) is gone, and their hand-written test
    descriptions are declared through the macro. Done looks like: a field of any shape is written
    as nested brackets matching its dimensions, read back to the same bytes, refused when the
    text's shape is not the field's, and every scene the existing writer and reader handled is
    unchanged.

    **1. The writer, `authoring/src/scene_write.c`.** Walk `field->rank` and `field->dims`:
    - **Rank 0**: the element, as today. **Otherwise** one `[` … `]` per dimension, outermost
      first, `, ` between items, no blank inside the brackets; a leaf of a vector kind brings its
      own `[x, y, z]`.
    - **`CHAR`**: the innermost dimension is one string. `CHAR, 32` writes `"…"` as today;
      `CHAR, 3, 8` writes `["a", "b", ""]`.
    - **`ENTITY`**: each element maps to its authored id as today.
    - Every refusal and warning that names a field also names the element's index, `tags[1]`.
    - A loop or recursion over the description, either: the description is the program's own, not
      read from a file, so rule 14 does not bind. Say which in the function's comment.

    **2. The reader, `authoring/src/scene_read.c`.** A value is read against the field's shape:
    - **The grammar is ADR-0154 point 1**: a value is an integer, a float, a boolean, a string, a
      name, or an array; an array is `[`, values separated by `,`, then `]`, and any array item may
      be an array; blanks around items and brackets are tolerated.
    - **An explicit stack and a named limit of 8 bracket levels** (rule 14), with a file-header
      comment saying why. Past 8 refuses the file, the message naming the limit — before any count
      is checked. A vector kind's own bracket counts as a level.
    - **Every level has exactly its dimension's count**, and every leaf reads as the field's kind:
      a ragged `[[1], [1, 2]]` and a mixed `[1, "a"]` refuse, with the line.
    - **A string inside an array** is `"…"` with the escapes `\"` and `\\` and nothing else, and
      `,` and `]` inside it are bytes. The sectioned parser (`assets/include/assets/sectioned.h`)
      does not check escapes inside an array; this does.
    - **`ENTITY` elements** are held as authored ids for pass two, each one patched as the existing
      reader patches one.

    **3. Headers and map.** `authoring/include/authoring/scene_write.h` and
    `authoring/include/authoring/scene_read.h`: the paragraph on values says arrays nest as the
    field's shape, `CHAR`'s innermost dimension is the string, the reader's limit is 8, ADR-0154.
    `authoring/authoring.md`: one line if it describes values (today the `tests/scene_read.c` line
    mentions "fixed arrays of a vector kind" — keep it true).

    **4. `authoring/tests/`.**
    - **The existing writer test's every-kind component (`every`, `every_rows` in
      `tests/scene_write.c`) and the existing reader test's array test component (`shaped`,
      `shaped_rows` in `tests/scene_read.c`) are declared through `VOE_BASE_DESCRIBE_STRUCT`**; the
      hand-written description tables go; their expected texts stand.
    - One test-only described component for shapes, used by both tests:
      - `F(int32_t, pair, INT32, 2, 3)` holding 1–6 → `pair = [[1, 2, 3], [4, 5, 6]]`
      - `F(voe_math_float3, points, FLOAT3, 2)` → `[[0, 0, 0], [1, 2.5, -3]]`
      - `F(voe_math_float3, grid, FLOAT3, 2, 2)` → three levels of brackets
      - `F(char, tags, CHAR, 3, 8)` holding `a`, `x"y\z`, empty → `["a", "x\"y\\z", ""]`
      - `F(uint8_t, deep, UINT8, 1, 1, 1, 1, 1, 1, 2)` → `[[[[[[[0, 7]]]]]]]`
      - `F(voe_math_float3, deepest, FLOAT3, 1, 1, 1, 1, 1, 1, 1)` → eight `[` in a row
      - `F(voe_ecs_entity, links, ENTITY, 2)` naming an authored entity and a dead one → `[3, 0]`
    - **Write**: that component's section byte for byte; a `tags` element holding a byte below
      `0x20` refuses and names `tags[n]`; **the existing writer test's three-entity expected text
      (`test_three_entities` in `tests/scene_write.c`) is unchanged.**
    - **Round trip**, both directions as the existing reader test does them, with the shapes
      component on two entities and `links` naming each other: bytes identical text-first, rows
      identical world-first.
    - **Read, each refusing and leaving zero entities**: `pair` as `[[1, 2, 3], [4, 5]]`, as
      `[1, 2, 3, 4, 5, 6]`, as `[[1, 2, 3], [4, 5, "6"]]`; `tags` as `["a", "C:\x", ""]`, and with an
      eight-byte string, one too long for its slot; `deepest` with a ninth level; `points` as `[]`.
    - **Read, tolerated**: `pair = [ [1,2,3] ,[4, 5,6] ]` loads and writes back canonical.

    **Before finishing**: `grep -rn 'voe_base_field_description.*=\s*{' authoring/tests` — every
    table it still finds has a comment saying why the macro cannot declare it. Include the shapes
    component's written section in your report.

    **What must not change.**
    - `base`, `scene`, `ecs`, `assets`. A gap in `describe.h`'s shape is a report
      (`BLOCKED: base, <why>`), not an edit.
    - The text written for, and the reading of, every field of rank 0 and every single string.
    - The existing reader's two passes, its kept sections, its refusals and warnings; no file I/O,
      no editor call site.
  - Covers: 3, 4, 5, 6, 7, 8
  - Depends on: 1
  - Done when: `cmake -P check.cmake` exits 0 on Linux, and
    `ctest --test-dir build/check/root -R authoring` passes.
