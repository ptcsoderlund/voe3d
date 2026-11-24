# 26 — Removing a kind from a landscape
folder: assets
after: 25
decisions: 0168, 0516, 0517

## Change
`assets/src/landscape_kinds.c` and `assets/include/assets/landscape.h`: new
`void voe_assets_landscape_kind_remove(voe_assets_landscape *, uint32_t kind)`. Asserts the kind is below
`kind_count`. It:
- makes every later kind's seed explicit (look `seed` = its `_kind_seed` + 1) before anything moves;
- drops the kind's path, look and every layer's `grows` column, moving later kinds down one;
- clears the freed last slot (empty path, zeroed look, no growth) and lowers `kind_count`;
- drops the kind's copies, keeping the others in order, and lowers each later copy's kind by one.

Nothing allocates. Header points: what goes, what moves, that later kinds keep their seeds, so their draws
stay where they were. Index entry in `assets/assets.md`.

`assets/tests/landscape_kinds.c`, new tests:
- three kinds with copies of each, growth on two layers; remove kind 1. Two kinds remain, the old kind 2
  now 1 with its path, look, growth and seed 2. Its copies are the old ones in order, kind 1; none of the
  removed kind's are left;
- that landscape written and read back strictly gives the same landscape;
- removing the last kind leaves the others untouched;
- after a removal, `seed_new` gives a seed no kind has.

## Done when
`ctest --test-dir build/debug -R '^assets/landscape_kinds$'` exits 0 with the new tests in it.
