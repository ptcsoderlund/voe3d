# 002 Fields of any shape

Status: approved
Approved: 2026-09-14
Accepted: -

A component field can be an array of any kind — numbers, vectors, strings, entity references —
with up to seven fixed dimensions, and a scene file saves and loads it as nested brackets. A
component stays plain bytes, so saving, undo and loading still copy it for free. The sponsor wants
arrays in the text format whose items can be anything a value can be, arrays included.

## Acceptance criteria

1. A field declared as a 4-by-2 grid of vectors, or as eight names of 32 bytes, compiles to
   exactly the struct C would give for the same array, and every existing component is the same
   size and layout as before.
2. Declaring a field with a dimension of 0, or a type that does not match its kind, fails the build
   with a message that says what is wrong.
3. A scene with array fields of one, two and seven dimensions — integers, vectors, strings and
   entity references, including a 2-by-2 grid of vectors that nests three brackets deep and a
   seven-dimensional vector field that nests eight — saves as nested brackets matching each
   field's dimensions, for example
   `pair = [[1, 2, 3], [4, 5, 6]]` and `tags = ["a", "x\"y\\z", ""]`.
4. Saving then loading that scene gives back identical rows, and loading then saving gives back
   identical text.
5. Loading refuses the whole file, naming the line, when an array's shape does not match its field:
   a row too short (`[[1, 2, 3], [4, 5]]`), flattened (`[1, 2, 3, 4, 5, 6]`), mixed
   (`[[1, 2, 3], [4, 5, "6"]]`), empty (`[]`), a string too long for its slot, a bad escape, or more
   than eight levels of brackets.
6. Extra spaces inside brackets load, and are written back in the canonical spelling.
7. Every scene that saved and loaded before this feature saves and loads unchanged.
8. `cmake -P check.cmake` exits zero on Linux.

## Out of scope

- Arrays that grow or mix kinds, as in JSON.
- Showing or editing array fields in the inspector.
- Any real component gaining an array field; the shapes are proven by test components.

## Constraints

- Linux first: acceptance is on Linux.

## Defaults

- Brackets nest at most eight levels deep in a file, so a broken or hostile file cannot crash the
  reader.
- A field has at most seven dimensions. For strings, the innermost dimension is the string's
  length in bytes.
- A list shorter than its slots carries its own count in a separate field, as a C struct would.

## Open questions

- None.
