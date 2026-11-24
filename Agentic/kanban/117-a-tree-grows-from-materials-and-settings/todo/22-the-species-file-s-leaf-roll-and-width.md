# 22 — The species file's leaf roll and leaf width
folder: assets
after: none
decisions: 0168, 0516, 0517, 0515, 0511, 0512

## Change
`assets/include/assets/tree.h` and `assets/src/tree.c`: two keys after `leaf_density`, each as the other
number keys are:
- `leaf_roll`, a float 0–1, default 0 (0517 point 1);
- `leaf_width`, a float 0.1–4, default 1, the card's width as a share of `leaf_size`.

For each:
- a field in `voe_assets_tree` after `leaf_density`, before `variations`;
- MIN, MAX and DEFAULT constants;
- the lenient read clamps, a missing key reads the default;
- the strict read refuses a value out of range;
- the writer writes it;
- a row in the key table `voe_assets_tree_keys` (it feeds `Docs/`).

Header points to add: what each key does to a card (roll pulls it toward the twig's level plane, width
sets its edge out from the twig); a file without them grows as before.

Update the tree entry in `assets/assets.md` if it lists the keys.

`assets/tests/tree.c`, new tests:
- a file without the keys reads roll 0 and width 1;
- the lenient read clamps `leaf_roll=2` to 1 and `leaf_width=0` to 0.1;
- the strict read refuses both;
- written and read back, roll 0.5 and width 2.5 hold;
- the key table has two more rows, named `leaf_roll` and `leaf_width`.

## Done when
`ctest --test-dir build/debug -R '^assets/tree$'` exits 0 with the new tests in it.
