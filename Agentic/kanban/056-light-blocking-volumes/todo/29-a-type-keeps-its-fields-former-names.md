# 29 — A type keeps its fields' former names
folder: ecs
after: none
decisions: 0168, 0324, 0353

## Change
0353 point 2: ecs stores a described type's former field names beside its unsaid row, and hands
them back; it never reads them. Read the headers of `ecs/include/ecs/component.h` (the unsaid row
and menu path sections are the pattern), `ecs/src/world_internal.h` and `ecs/src/component.c`.

- `ecs/include/ecs/component.h`, after the unsaid row's section:
  - `typedef struct { const char *former; const char *field; } voe_ecs_former_name;`
  - `void voe_ecs_component_formerly_set(voe_ecs_world *world, voe_ecs_type type, const
    voe_ecs_former_name *names, uint32_t count);` Once per type; a second call, a NULL list, a
    count of 0 or a NULL string asserts. The list is not copied: it is the declaring folder's and
    outlives the world, as a menu path does.
  - `const voe_ecs_former_name *voe_ecs_component_formerly(const voe_ecs_world *world, voe_ecs_type
    type, uint32_t *count);` The list and its count, or NULL and 0 when none was set.
  - Section comment points: what a former name is for (a field renamed, so an old file's key
    still reaches it); ecs never reads it, the reader of files does; 0353.
- `ecs/src/world_internal.h`: the type's record gains the list and its count beside `unsaid_row`
  and `menu`; its comment says so.
- `ecs/src/component.c`: the two calls, as `_menu_set` and `_menu` are written; header point if
  its list of what a type holds names them.
- `ecs/tests/component.c`: a case: a type with no former names hands back NULL and 0; after
  `_formerly_set` with two pairs, the same pointer and 2.
- `ecs/ecs.md` (the component.h entry names former field names), `ecs/tests/tests.md` (the
  component.c entry names the case). Each at most 300 characters.

## Done when
The test `ecs/component` passes after the folder's build.
