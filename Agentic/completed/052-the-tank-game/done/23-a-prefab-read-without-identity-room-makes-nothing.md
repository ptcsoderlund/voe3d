# 23 — A prefab read without room for its identities makes nothing
folder: authoring
after: 22
decisions: 0168, 0283, 0335, 0337

## Change
0337 point 4, the reader's half. Read `authoring/include/authoring/prefab.h`,
`authoring/src/prefab_read.c` (its header first), `authoring/tests/prefab.c`,
`authoring/src/src.md` and `authoring/tests/tests.md`.

- `prefab_read.c`: after the text is validated whole and before any entity is made, count the
  entities the read would make (the file's entities less its root, which `root` stands for) and
  refuse, reported through `VOE_BASE_ERROR` as the file's other refusals are, saying the world has
  no room for the prefab's identities and nothing was loaded, when `voe_ecs_component_count` of the identity type plus that
  count exceeds `voe_ecs_component_capacity` (ecs/component.h, card 22). Refused, the world is
  untouched: no entity, no row, not even the root's.
- `prefab.h`: `voe_authoring_prefab_read`'s comment gains the refusal, untouched, when the identity
  table cannot take every entity it would make, because an editor entity always has an identity
  (0335); the "half-made" sentence stays for the other tables. The cook paragraph's "the identity
  table stays at 32" says the table stays small, with no number.
- `tests/prefab.c`: a new check: a world whose identity table has room for one entity fewer than a
  three-entity prefab needs refuses the read, with the entity count and the identity count unchanged
  and the root holding no part row; the same text with room enough reads.
- `src.md` and `tests/tests.md`: prefab_read.c's and prefab.c's entries name the refusal.

## Done when
The test `authoring/prefab` passes with the new room check in it.
