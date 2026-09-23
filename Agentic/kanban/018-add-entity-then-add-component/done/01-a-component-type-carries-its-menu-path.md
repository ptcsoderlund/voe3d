# 01 — A component type carries its menu path
folder: ecs
decisions: 0168, 0217, 0221

## Change
Add a menu path to what a world keeps per component type, beside the needed type (0193), stored and handed
back and never read by `ecs`.

- `ecs/include/ecs/component.h` — after `voe_ecs_component_needs`, a paragraph in the style of the needs one:
  what the path is for (where a tool offers the type, 0217), that it is parts split on `/`, that `ecs` never
  parses or reads it, that the string must outlive the world. Two declarations:
  - `void voe_ecs_component_menu_set(voe_ecs_world *world, voe_ecs_type type, const char *path);` — once per
    type; a second call, a NULL or empty `path` asserts.
  - `const char *voe_ecs_component_menu(const voe_ecs_world *world, voe_ecs_type type);` — the path, or NULL
    when none was set.
- `ecs/src/component.c` — both, storing the pointer where the needed type is stored (the per-type record,
  which may be in `ecs/src/world_internal.h`; change that file if the record is there).
- `ecs/tests/component.c` — one new test function `menu_path`: a type with no path reads NULL; after
  `_menu_set` it reads back the same pointer; a second type is unaffected. Call it from `main`.
- `ecs/ecs.md` — the `component.h` entry's list of what a type carries gains "menu path".
- `ecs/src/src.md`, `ecs/tests/tests.md` — only if their entries for these files list what they hold.

## Done when
`ctest --test-dir build/debug -R '^ecs/component$'` passes with `menu_path` in it, and the Checks line of
`CLAUDE.md` with `{folder}` = `ecs` exits 0.
