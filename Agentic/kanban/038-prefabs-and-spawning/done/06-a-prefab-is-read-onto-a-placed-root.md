# 06 — A prefab is read onto a placed root
folder: authoring
decisions: 0168, 0283, 0152

## Change
Needs card 05. 0283 point 4: the load the editor expands a placed copy with.

- `authoring/include/authoring/prefab.h`: `[[nodiscard]] bool voe_authoring_prefab_read(
  const char *text, size_t size, voe_ecs_world *world, voe_ecs_entity root, uint64_t first_id,
  voe_base_arena *arena, uint64_t *out_next_id);`. Validates the whole text first, as
  scene_read.h does, and also refuses a text without exactly one entity with no parent section,
  or holding a camera, light, prefab or part section; refused, it creates nothing. Then: `root`
  (alive, with a transform) is given every row of the prefab's root it does not already have,
  never identity, transform or parent; every other entity in the file is created, in ascending
  file id, with an identity of id `first_id`, `first_id + 1`, … and the file's name, and all its
  rows, ENTITY fields naming a file id mapped to the entity made for it (the prefab root's id to
  `root`), any other to zero; `root` and every made entity get a `voe_scene_prefab_part` row
  naming `root`. `*out_next_id` is the id after the last one used. A section naming an
  unregistered type is skipped with one warning per type, not kept. Running out of room returns
  false with the world half-made, as scene_read.
  Header points: it is a load, rule 3's creation exception, adding rows directly to entities it
  just made and to a root the same load made; it never overwrites a row the root has.
- `authoring/src/prefab_read.c` (new): reuse `authoring/src/scene_read.c`'s validating pass and
  `authoring/src/field_read.h`'s value reading through a private header shared with
  `scene_read.c`, not a copy. `scene_read.c`'s behaviour is unchanged.
- `authoring/tests/prefab.c`: the three-part hull written by `voe_authoring_prefab_write`, read
  onto a fresh root with a transform at (3, 0, 5) and first id 100: the root has the hull's
  shape and keeps its own transform; turret and barrel exist with ids 100 and 101, parented as
  in the file; all three carry part rows naming the root; out id 102. Refused: two roots, a
  camera section, a prefab section; after a refusal the world's entity count is unchanged.
- `authoring/src/src.md`, `authoring/tests/tests.md`, `authoring/authoring.md`: entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_authoring $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_authoring_[A-Za-z0-9_]+") && ctest --test-dir build/debug
-R "^authoring/"` exits 0, `voe_test_authoring_prefab` and `voe_test_authoring_scene_read`
among them.
