# 09 — Play and Ship cook each kind's look and each layer's growth
folder: editor
after: 08, 03
decisions: 0168, 0236, 0476

## Change
0476 point 4's cook: the game tree's `landscapes.c` names each landscape's kinds' looks and its
layers' growth in card 03's `voe_game_landscape` fields, so Play and a shipped game grow what the
editor grows.

- `editor/src/game_tree_landscapes.c` —
  - `landscape_<n>_kind_sizes` becomes `static const voe_assets_landscape_look
    landscape_<n>_looks[] = { {size, size_vary, turn_vary, tilt, shadowless}, ... };`, one per
    kind, written when the landscape has kinds, named in the entry's `looks` (NULL with none).
  - New `static const uint16_t landscape_<n>_grows[] = { ... };`, layer_count × kind_count
    densities layer by layer, written when any is above 0 and named in the entry's last field
    `grows`, NULL otherwise.
  - The header's sketch and its paragraph on what each entry names follow.
- `editor/src/src.md` — the `game_tree_landscapes.c` entry names looks and growth.

## Done when
The editor builds, and `grep -n "_grows" editor/src/game_tree_landscapes.c` prints a line.
