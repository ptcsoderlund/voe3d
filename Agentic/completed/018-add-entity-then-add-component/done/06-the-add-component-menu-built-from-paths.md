# 06 — The Add component menu, built from the types' paths
folder: editor
decisions: 0168, 0217, 0221

## Change
A new pair, `editor/src/add_menu.h` and `editor/src/add_menu.c`: the entries Add component offers one entity,
as a tree built every frame from the world's types. Data only; no `ui`. Card 07 draws it.

`add_menu.h` — header points: what the menu is and that it is built from `voe_ecs_component_menu`
(`ecs/include/ecs/component.h`) so the editor names no component (0217); which types are offered (0221: described,
with a default row and a path, and the entity lacks it); how a path splits (on `/`, parts trimmed); that
groups come in the order their first entry's type was registered and entries in registration order; that a
group exists only if it holds an offered entry; that a malformed path or one past the limits asserts, a path
being the program's own.

- `VOE_EDITOR_ADD_MENU_DEPTH` 4 (levels, the entry's included), `VOE_EDITOR_ADD_MENU_ENTRIES` 32,
  `VOE_EDITOR_ADD_MENU_LABEL` 32 (bytes of a part, its zero included), `VOE_EDITOR_ADD_MENU_TOP` as the parent
  of a top-level entry (`UINT32_MAX`).
- `voe_editor_add_menu_entry { char label[VOE_EDITOR_ADD_MENU_LABEL]; uint32_t parent; bool group;
  voe_ecs_type type; }` — `type` meaningful only when not a group; `parent` an index into the same array.
- `voe_editor_add_menu { voe_editor_add_menu_entry entries[VOE_EDITOR_ADD_MENU_ENTRIES]; uint32_t count; }`.
- `void voe_editor_add_menu_build(voe_editor_add_menu *menu, const voe_ecs_world *world,
  voe_ecs_entity entity);` — clears and fills `menu`. A group is found by label under the same parent before a
  new one is made, so "Rendering / Light" and "Rendering / Shape" share one "Rendering".

`add_menu.c` carries it out; its opening says what the file does.

`editor/src/src.md` — entries for the two new files.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and
`grep -q "add_menu.c" build/debug/build.ninja` exits 0.
