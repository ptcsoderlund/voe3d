# 25 — A foliage kind's seed
folder: assets
after: 22
decisions: 0168, 0517, 0516, 0476, 0442, 0511

## Change
`assets/include/assets/landscape.h`:
- `voe_assets_landscape_look` gains a last field, `uint8_t seed`: the kind's seed plus one, 0 meaning the
  kind's own index (0517 point 4). The look default leaves it 0. It is read only through `_kind_seed`.
- New functions, beside `voe_assets_landscape_kind_size`:
  - `uint32_t voe_assets_landscape_kind_seed(const voe_assets_landscape *, uint32_t kind)`: the kind's
    seed. Asserts the kind is below `VOE_ASSETS_LANDSCAPE_KINDS`.
  - `uint32_t voe_assets_landscape_seed_new(const voe_assets_landscape *)`: the seed a kind added at
    `kind_count` takes: `kind_count` if no kind has it, else the least seed no kind has.

New `assets/src/landscape_kinds.c` holds both, with its header comment. It is the home of card 26's
removal too.

`assets/src/landscape_foliage.c`: the look key `seed<k>` in `[Foliage]`, 0 to `KINDS` − 1, as the other
look keys are:
- the lenient read clamps and a missing key leaves 0;
- the strict read refuses a value out of range;
- the writer writes it only when the seed is not k.

`assets/src/landscape_keys.c`: a row for `seed<k>` (it feeds `Docs/`). Header points on the look keys
and on writing only off the default gain the seed. Index entries in `assets/assets.md`: the new file.

New `assets/tests/landscape_kinds.c`:
- a file with three kinds and no seed keys reads seeds 0, 1, 2, and writes no `seed` line;
- a look with seed 5 on kind 1 writes `seed1`, reads back 5;
- the lenient read clamps `seed0=99` to 15; the strict read refuses it;
- `seed_new` is `kind_count` on a fresh landscape, and the least free seed when that is taken.

`assets/tests/landscape_keys.c`: the table has the `seed<k>` row.

## Done when
`ctest --test-dir build/debug -R '^assets/landscape_'` exits 0 with `assets/landscape_kinds` in it.
