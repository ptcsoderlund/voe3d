# 41 — The editor grows and cooks no species
folder: editor
after: 40
decisions: 0168, 0520

## Change
Take out of `editor/` what these commits added, seen with `git show <hash> -- editor`: card 08 `4c90248f`
(the editor grows the project's species), card 17 `1bd8dd30` (the game tree cooks the trees table) and
card 24 `7167f7d6`'s part in `game_tree_materials.c`. Later cards' changes to the same files stay.

- Delete `editor/src/trees.c` and `editor/src/trees.h`.
- `editor/src/models.c`, `models.h`, `models_store.h`: the store no longer grows species or calls 3d's
  species, grown or variation calls; a kind's model is its glb's, loaded as any glb is.
- `editor/src/game_tree.h`, `game_tree_materials.c`: no trees table and no species cook.

After this card nothing in the editor includes `game/trees.h`, `assets/tree.h` or `3d/tree_grow.h`, so
cards 42 to 44 can delete them. Header points and `editor/src/src.md` entries naming species go.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -rnE 'game/trees\.h|assets/tree\.h|3d/tree_grow\.h|species|put_grown|models_variation' editor/src`
prints nothing.
