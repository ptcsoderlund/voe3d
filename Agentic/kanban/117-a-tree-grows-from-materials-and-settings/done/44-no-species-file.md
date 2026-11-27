# 44 — No species file
folder: assets
after: 43, 36
decisions: 0168, 0520

## Change
Take out of `assets/` what card 01 `fc291402` and card 22 `ed517d05` added, seen with
`git show <hash> -- assets`.

- Delete `assets/include/assets/tree.h`, `assets/src/tree.c`, `assets/tests/tree.c`.
- `assets/include/assets/landscape.h`: card 01's mention of `.tree` kind paths goes.
- Entries in `assets/assets.md`, `assets/include/assets/assets.md`, `assets/src/src.md`,
  `assets/tests/tests.md`.

## Done when
`cmake --build --preset debug --target voe_assets` exits 0, and
`git grep -lE 'assets/tree\.h|voe_assets_tree' -- ':!history' ':!Agentic'` prints nothing.
