# 01 — A thing can be a placed prefab, or a part of one
folder: scene
decisions: 0168, 0283, 0281

## Change
The two rows of 0283 point 2, in the shape `parent_component.h` / `parent_system.h` already have.
Read those two headers first as the pattern.

- `scene/include/scene/prefab_component.h` (new): `VOE_SCENE_PREFAB_PATH` 128;
  `voe_scene_prefab`, described, one CHAR field `path` of that size; `voe_scene_prefab_part`,
  a plain struct (not described: runtime-only) with one `voe_ecs_entity instance`; the two keys
  `voe_scene_prefab_key` and `voe_scene_prefab_part_key` (VOE_BASE_IMPORTED);
  `const voe_scene_prefab *voe_scene_prefab_get(const voe_ecs_world *, voe_ecs_entity)` and
  `const voe_scene_prefab_part *voe_scene_prefab_part_get(...)`, NULL for no table or no row.
  Header points: the path is project-relative with `/`, as a model's; the prefab row marks a
  placed copy's root and is saved; the part row is on everything made from a prefab, the root
  naming itself, and is never saved; what a placed copy saves (0283 point 3) and who expands it
  (the editor, 0283 point 4); why parts carry identities (Scene list, pick, undo's re-find).
- `scene/include/scene/prefab_system.h` (new): `void voe_scene_prefab_register(voe_ecs_world *,
  uint32_t capacity)` registers both tables at that capacity: the prefab row with its
  description, a default row (empty path), needing a transform, no replace, no menu; the part
  row with `voe_ecs_runtime_only`, needing a transform. Header points: no replace or menu, so Add
  component never offers either and the Inspector shows the path without editing it; rows are
  added through the structural queue or by a load (rule 3).
- `scene/src/prefab_component.c`, `scene/src/prefab_system.c` (new).
- `scene/tests/prefab.c` (new): on a world with transforms registered, both register; an entity
  given both rows through `voe_ecs_structure_add` and applied reads them back through the two
  getters; the part table is runtime-only and the prefab table is not; the prefab type has no
  replace and no menu; getters answer NULL on an entity with no row.
- `scene/scene.md`, `scene/include/scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`:
  one entry per new file.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_scene $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_scene_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^scene/"` exits 0, `voe_test_scene_prefab` among them.
