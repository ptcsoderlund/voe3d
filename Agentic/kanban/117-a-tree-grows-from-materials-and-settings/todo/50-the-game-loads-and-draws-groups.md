# 50 — The game loads and draws groups
folder: game
after: 49, 47, 42
decisions: 0168, 0520, 0521

## Change
`game/include/game/landscapes.h`: the cooked `voe_game_landscape` loses `kinds`, `kind_count` and `looks`
and gains `groups` (`const voe_assets_landscape_group *`) and `group_count`; `grows` are by group
(0521 point 7).

`game/src/models.c` (and `models_cooked.c` if it reads them): `voe_game_models_kinds_gather` becomes a
gather of every glb path of every group, named for that; each group's glbs, looks and grows go to the
store through 3d's `voe_3d_models_landscape_glb_set`, `_look_set`, `_grow_set`
(`3d/include/3d/models_foliage.h`).

Tests: card 38's split `game/tests/models*.c` build on groups, expectations unchanged. New test: a cooked
landscape with one group of two glbs loads both glbs' models into the store.

Header points naming kinds say groups. Entries in `game/include/game/game.md`, `game/src/src.md`,
`game/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R '^game/models'` exits 0 after building `voe_game` and those test targets,
and `git grep -nE 'kind_count|kinds_gather|KIND_PATH' -- game` prints nothing.
