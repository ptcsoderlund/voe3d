# 0154. An array item is any value, and a described field declares its shape

- **Status:** Accepted
- **Date:** 2026-09-13
- **Deciders:** Human, Tech Lead
- **Supersedes:** in part — ADR-0149's *Fixed arrays* row (*an array of vectors nests once*);
  ADR-0127 point 1's *element count*; ADR-0123 point 5, as far as an array's elements go
- **Superseded by:** —

## Context

The principal, 2026-09-13, after card 070's coder found that no real component could declare
the nested array ADR-0149's value table spells:

> *"Our textbased sections file format. We want to allow arrays if we dont do it yet. an array
> item can hold the same as our other values. (primitives, string and array) which means we want
> to be able to hold arrays in arrays in arrays ..."*

**What existed.** ADR-0149's table: `[a, b, c]`, and an array of vectors nests once. No array of
strings, no deeper nesting. In code, `base/include/base/describe.h` lets only `ENUM`, `CHAR` and
`ENTITY` repeat, and a description carries one element count. Card 070's writer (in review)
writes one level.

**The text is not the hard part; the storage is.** ADR-0123 made a component flat — primitives,
fixed-size arrays, an entity reference, *nothing that allocates, nothing that nests* — and that
is what makes a component memcpy-able for the save, undo and the loader thread, and what lets the
world's arena be freed by rewinding. The question is what nested text becomes in a row.

Constraints. Rule 14: no recursion over data read from a file — an explicit stack and a named
limit. ADR-0073: an edit touches one line. ADR-0149: one spelling per value, never zero-filled,
the value spellings apply to every authored file. ADR-0127 point 2: the declared type is checked
against the kind at compile time.

## Options considered

### Option A — nested text, stored as fixed-size arrays
An array item is any value, arrays included, to a depth limit. A field is one kind and a shape of
fixed dimensions; every level has an exact count. Still flat bytes.

### Option B — like JSON: arrays of any length and mixed items
The same text, with components able to hold a growing array whose items differ in kind. Each item
needs a tag, the array needs memory that grows, and a row stops being plain bytes.

## Decision

**Option A**, the tech lead's recommendation and the principal's call.

**The text** — every authored file:

1. **A value is an integer, a float, a boolean, a string, a name, or an array. An array is `[`,
   values separated by `,`, then `]`, and any of its values may be an array.** That is the whole
   grammar; the field's description, not the text, says what each value must be.
2. **Blanks around items and brackets are tolerated on read; the writer emits `[a, b]`** — `, `
   between items, none inside the brackets. A value is one line, however long.
3. **Nesting is refused past 8 bracket levels**, a named constant in the reader, walked with an
   explicit stack (rule 14). A vector kind's own bracket counts as a level.
4. **A string inside an array is the same string**: `"..."`, escapes `\"` and `\\` and nothing
   else; `,` and `]` inside it are bytes. The sectioned parser strips quotes only from a value
   that starts with `"`, so it hands an array back as raw text and changes in no respect; the
   scene reader checks the escapes inside an array.

**The storage** — what a described field may be:

5. **A field is one kind and a shape: 0 to 7 dimensions, outermost first, each at least 1.** No
   dimensions is one value. With a vector kind's own bracket, 7 is the 8 levels of point 3.
6. **Every kind may be an array.** `describe.h`'s three-kind restriction goes.
7. **Rectangular and single-kind.** Every level of the text has exactly the count its dimension
   says, and every leaf is the field's kind. `[[1], [1, 2]]` and `[1, "a"]` refuse the file with
   the line, as a wrong count does today. Never zero-filled.
8. **For `CHAR`, the innermost dimension is the string's bytes.** `CHAR, 32` is one string;
   `CHAR, 8, 32` is eight strings, written `["a", "b", …]`. A string shorter than its slot is
   normal, as today.
9. **A field line names one element's type, then the kind, then the dimensions:**
   `F(voe_math_float3, path, FLOAT3, 4, 2)` declares `voe_math_float3 path[4][2]`, and
   `F(char[32], name, CHAR)` becomes `F(char, name, CHAR, 32)`. The type's size is checked against
   the kind exactly, every field. The dimensions are spelled out because C cannot recover an
   array's dimensions from its type in a constant expression.
10. **A description keeps `count` as the total number of elements** — what every reader of it
    uses today — and gains the rank and the dimensions.

**What a writer emits** is ADR-0149 points 5 and 6 unchanged, nesting as the shape says:
`FLOAT3, 4, 2` is `[[[x, y, z], [x, y, z]], …]`, four of those.

## Blast radius

**Point 1 is load-bearing once files use it**, like every spelling in ADR-0149 — but it only
widens: every file valid today stays valid. **Points 5–10 are cheap today**: two declarations in
the tree have an array (`scene`'s identity name and a `base` test), the writer in review is the
one reader of `count` beyond them, and the inspector ignores arrays. They get dearer with every
component written. Reversibility: **moderate.**

## Consequences

- **Every shape the principal named can be held** — arrays of numbers, of strings, of arrays —
  without a component ceasing to be plain bytes. Save, undo and the loader thread keep their copy.
- **A list shorter than its capacity is the component's own business**: a fixed array and a count
  field beside it, which is how a flat struct has always done it. The file writes every slot.
- **A big array is a long line.** One line per value keeps an edit local (ADR-0073); an array of
  sixteen matrices is a line of 256 numbers. Accepted.
- **Every field line with an array changes spelling**, and a field line gains up to seven numbers.
- **The inspector cannot show an array other than a string yet** — D-266.
- **Card order.** 071, the reader, was claimed while this was being written and reads one level as
  ADR-0149 had it. 077 (`base`: the shape) has nothing in front of it; 078 (`authoring` writes and
  reads any shape) follows 070, 071 and 077 and removes their one-level code.

## Rejected options and why

**B — like JSON.** It reverses ADR-0123: a tagged, growing array is not memcpy-able, needs memory
the world's arena cannot rewind, and costs a deep copy in the save, undo and the loader thread.
Only a reader that knows its schema — the theme — could use it, and a scene could not. A growing
list already has homes: an entity per item, or a child naming its parent (ADR-0124).

**One level, as ADR-0149 had it.** What the principal asked to change.

**The shape spelled in the type, `F(voe_math_float3[4][2], path, FLOAT3)`.** It reads like C, but
the macro could check only the total size and could not fill in the dimensions, so `[8][32]` and
`[32][8]` would describe the same bytes and write different files.

**No depth limit.** Rule 14.

## Questions this opens

- **D-266** — how the inspector shows an array field: a row per element, a collapsed row that
  opens, or a count and nothing else; and how a field of seven dimensions stays usable. Nothing
  needs it until a real component declares an array other than a string.
