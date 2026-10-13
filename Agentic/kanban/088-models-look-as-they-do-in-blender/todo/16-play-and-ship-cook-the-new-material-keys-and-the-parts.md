# 16 — Play and Ship cook the new material keys and the parts
folder: editor
after: 15
decisions: 0168, 0236, 0420

## Change
The game tree's `materials.c` carries card 04's keys and defines `voe_game_parts_cooked` (card 14), so
Play and Ship look like the editor (How to test 8).

- `editor/src/game_tree_materials.c`: each material's initializer also writes `.cutout`,
  `.double_sided`, `.emission`, `.emission_strength` (floats `%a` as the rest) and `.emissionmap`
  (escaped as the other maps). After the materials, `voe_game_parts_cooked`: every `.parts` under
  `Assets/` found as `editor/src/game_tree_find.h` finds files, read with `assets/parts.h`, one
  `voe_game_model_parts` each with its `.glb` path and its `count` and material paths; none is
  `{ NULL, 0 }`; a bad file refused naming it, as a bad `.material` is. The header's example shows both
  tables.
- `editor/src/game_tree.h`: the LAYOUT and the `materials.c` sentences say it holds the parts too.
- `editor/src/src.md`: the `game_tree_materials.c` entry follows (at most 300 characters).

## Done when
`voe_editor` builds in the folder's check with no warning.
