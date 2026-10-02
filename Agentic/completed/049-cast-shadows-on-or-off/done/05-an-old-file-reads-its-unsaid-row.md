# 05 — An old file reads its unsaid row
folder: authoring
after: 01, 02
decisions: 0168, 0324

## Change
0324 point 3, the reader's half, and the cook carrying the light's flag (authoring does not link 3d; a
shape's or model's flag reaches files and the cook through its
description, proven by the editor's card). Read
`authoring/src/scene_read.c` (the fallback, around line 272, and its header
comment), `authoring/include/authoring/scene_read.h` (the missing-field
paragraph, around line 45), the unsaid-row pair in
`ecs/include/ecs/component.h`, `authoring/tests/scene_cook.c`,
`authoring/tests/tests.md`, `authoring/src/src.md`, `authoring/authoring.md`.

- `scene_read.c`: a field a section does not mention takes its bytes from
  the type's unsaid row (`voe_ecs_component_unsaid`) when it has one, else
  the default row, else zero; the warning names which it was ("its unsaid
  value" beside "its default" and "zero"). Prefab reading shares this pass
  and needs no change. Header comment says the order.
- `scene_read.h`: the missing-field paragraph says the unsaid row comes
  first, and why (0324: a light saved before 049 keeps casting).
- New `authoring/tests/scene_read_unsaid.c` (a test-only described type
  with a default row and an unsaid row that differ in one field, as
  `scene_read.c`'s test types are declared; plus scene's real light): a section without the field reads the unsaid value; a
  type with no unsaid row reads its default; a section that says the field
  keeps what it says; a `[N.voe_scene_light]` with no `cast_shadows` reads
  true, one with `cast_shadows = false` reads false. Header comment
  says what it proves. Do not grow `authoring/tests/scene_read.c`: it is
  over 800 lines.
- `authoring/tests/scene_cook.c`: the sun it adds sets `cast_shadows = true`;
  the expected source carries the flag as the cook spells a BOOL. Any
  other exact-text expectation in the authoring suite that now holds a
  light row is updated
  the same way. No file under `authoring/src/` other than `scene_read.c`
  changes; if a test fails for any reason but expected text, block the card
  with what failed.
- `authoring/tests/tests.md` (the new file; scene_cook.c's entry names the
  cooked flag), `authoring/src/src.md` and `authoring/authoring.md` where
  they no longer say what the file does.

## Done when
`ctest --test-dir build/debug -R '^authoring/'` passes, including
`authoring/scene_read_unsaid` and `authoring/scene_cook` with the sun's flag.
