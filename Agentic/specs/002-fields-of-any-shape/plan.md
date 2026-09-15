# 002 Fields of any shape — plan

A described field becomes one element type, one kind and zero to seven fixed dimensions, declared
through `base`'s field-list macros so every struct keeps its C layout byte for byte (task 1). Then
`authoring`'s writer and reader walk that shape: nested brackets out, an explicit stack with an
8-level limit in, every level's count and every leaf's kind checked, and the old one-level array
code removed (task 2). No real component gains an array; test components prove the shapes.

## Decisions

- An array item is any value; a field is one kind and 0–7 dimensions, outermost first; `count`
  stays the total element count; `CHAR`'s innermost dimension is the string's bytes; the reader
  refuses past 8 bracket levels — decided before this spec. Project-wide:
  `Agentic/decisions/0154-an-array-item-is-any-value-and-a-field-declares-its-shape.md`.
- Task 1 edits the two downstream declarations its new macro spelling breaks
  (`scene/include/scene/identity_component.h`, and `base/tests/describe.c` in its own folder) — the
  call-site licence in the project `CLAUDE.md` (*a card owns the call sites of a change it
  mandates*). No new folder edge.
- The writer may loop or recurse over the description (the program's own data, so rule 14 does not
  bind); the reader may not recurse over the text (rule 14). Feature-local.

## Folders

- `base/` — changed — `voe_base_field_description` gains `rank` and `dims[VOE_BASE_FIELD_RANK_MAX]`;
  `#define VOE_BASE_FIELD_RANK_MAX 7`; `F`/`F_READ_ONLY` take `(type, field, KIND, ...)` with zero
  to seven dimensions; the `VOE_BASE_FIELD_REPEATS_*` table is removed.
- `scene/` — changed — internal only: the identity component's field line respelled, layout
  unchanged.
- `authoring/` — changed — internal only: `scene_write`/`scene_read` signatures unchanged; their
  header paragraphs on values change.

## Verification

- `cmake -P check.cmake` — exits 0 on Linux after each task (acceptance criterion 8).
- `ctest --test-dir build/check/root -R 'describe|identity'` — passes: the shaped fields' rows
  and every existing struct's `sizeof`/`offsetof` match their hand-written twins (criterion 1).
- `ctest --test-dir build/check/root -R authoring` — passes: the shapes component's exact text,
  both round trips, every refusal leaving zero entities, the tolerated spacing, and the existing
  writer test's three-entity text unchanged (criteria 3, 4, 5, 6, 7).
- By hand, task 1 (criterion 2): in a scratch copy of `base/tests/describe.c`, declare
  `F(vector, bad, FLOAT2, 2)` and `F(int32_t, bad, INT32, 0)`, build, read that each compiler
  message says what is wrong (type size against kind; dimension of 0), put both messages in the
  report, and revert.
- By hand, task 1: `grep -rn 'F\(_READ_ONLY\)\?(.*\[' --include=*.h --include=*.c .` — any line it
  prints is not a field line (the report lists them).
- By hand, task 2: `grep -rn 'voe_base_field_description.*=\s*{' authoring/tests` — every table it
  still finds has a comment saying why the macro cannot declare it; the shapes component's written
  section is in the report and matches criterion 3's examples.
