# 01 — A type may say what an unsaid field means
folder: ecs
after: none
decisions: 0168, 0324

## Change
0324 point 3, the `ecs` half. Read `ecs/include/ecs/component.h` (the
default-row section, around line 207), `ecs/src/component.c`,
`ecs/tests/component.c`, `ecs/ecs.md`, `ecs/src/src.md`,
`ecs/tests/tests.md`.

- `component.h`: beside the default row, two functions shaped like its pair:
  `void voe_ecs_component_unsaid_set(voe_ecs_world *world, voe_ecs_type type, const void *row)`
  and `const void *voe_ecs_component_unsaid(const voe_ecs_world *world, voe_ecs_type type)`.
  Header points: the unsaid row is what a field a file does not mention
  stands for, and differs from the default row only where what a new row is
  and what an old file meant part ways (0324); optional, NULL when unset;
  copied once into memory the world pushes, a second set asserts; ecs never
  reads it, the reader of files does.
- `component.c`: store it per type exactly as the default row is stored.
- `ecs/tests/component.c`: unset reads NULL; a set row reads back
  byte for byte and is not the default row's memory; setting it leaves the
  default row as it was.
- `ecs.md` (component.h's entry lists the unsaid row beside the default
  row), `src/src.md` and `tests/tests.md` where their entries no longer say
  what the file does.

## Done when
`ctest --test-dir build/debug -R '^ecs/component$'` passes with the three
unsaid-row checks.
