# 03 — Scene text saves and reads an identity's fold
folder: authoring
after: 02
decisions: 0168, 0302

## Change
0302 point 2: card 02 gave `voe_scene_identity` a `folded` BOOL after
`name`. Scene text spells an identity's fields other than `id` as `[N]`'s
keys (`authoring/include/authoring/scene_read.h`), so a written scene now
holds `folded = false` under every `[N]`, and the tests' expected texts no
longer match.

Read `authoring/include/authoring/scene_write.h`, `scene_read.h`,
`prefab.h` and `scene_cook.h`, and the headers of `authoring/src/scene_write.c`,
`scene_read.c`, `scene_cook.c`, `prefab_write.c`, `prefab_read.c` and
`prefab_cook.c`. Then `authoring/tests/scene_write.c`,
`authoring/tests/scene_read.c`, `authoring/tests/scene_cook.c`,
`authoring/tests/prefab.c` and `authoring/tests/tests.md`.

- If any of those source files spells the identity's fields by name rather
  than walking its description, make it carry `folded` the way it carries
  `name`, so writing, reading, prefabs and the cook all keep it. Otherwise
  no source changes.
- Every expected text in the four tests that spells an identity gains its
  `folded` line where the writer puts it; an expected cooked struct gains
  the field.
- `authoring/tests/scene_write.c` or `scene_read.c`: a check that an entity
  with `folded` true writes `folded = true` and reads back true; a check that
  a scene text with no `folded` key (as every scene saved before 043) reads
  with no refusal and `folded` false.
- `authoring/tests/tests.md`: the entries for the tests that gained checks.

## Done when
The new checks pass in the folder's checks, and
`grep -q "folded = true" authoring/tests/scene_write.c authoring/tests/scene_read.c`
finds it in at least one of them.
