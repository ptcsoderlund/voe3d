# 28 — The landscape cook writes each kind's seed
folder: editor
after: 24, 26, 27
decisions: 0168, 0517

## Change
`editor/src/game_tree_landscapes.c`: each kind's look is cooked as a positional initializer
`{size, size_vary, turn_vary, tilt, shadowless, blends, blend_height}`. Add the look's `seed` byte last,
written as stored (0 when the kind has no seed key). Left out, a removed kind's later kinds would grow and
pick in Play and Ship where they would have before the removal, not where the editor shows them.

Update the example in the file's header comment, and its entry in `editor/src/src.md` if it lists the
look's fields.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -n 'seed' editor/src/game_tree_landscapes.c` finds the written field.
