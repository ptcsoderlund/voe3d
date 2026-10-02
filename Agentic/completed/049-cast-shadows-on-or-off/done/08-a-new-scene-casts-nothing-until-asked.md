# 08 — A new scene's light casts nothing until asked
folder: editor
after: 02, 03, 04, 05
decisions: 0168, 0316, 0324

## Change
0324 points 2 and 4 in the editor. The Inspector, undo, save, prefabs and
Play carry the three flags through their descriptions with no code here.
Read `editor/src/project.c` (the untitled light, around line 83, and the
untitled cube, around line 181), `editor/src/project.h` (the preview light,
around line 113), `editor/src/view.h` (the view light, around line 173),
`editor/src/src.md`.

- `project.c`: the untitled light names `.cast_shadows = false`, with the
  comment point that a new scene casts nothing (0316); the untitled cube's
  shape literal names `.cast_shadows = true`, since zero is false. The
  preview light is unchanged: a world with no light row casts none (0324
  point 4).
- `project.h`: the preview light's comment says it casts no shadows
  (0316, 0324), not that its shadows come with it.
- `view.h`: the view light's comment, "shadows included", says the
  preview light casts none.
- `editor/src/src.md` only where an entry no longer says what the file
  does.

## Done when
`grep -c "cast_shadows" editor/src/project.c` prints 2 or more, and
`ctest --test-dir build/debug -R '^editor/'` exits 0.

The human, in the editor, walks `## How to test` in `feature.md`, steps 1
to 8: the sun's and a house's `cast_shadows` box in `examples/tank_game`
and the shadows going and coming; undo and redo; both kept over a save and
reopen; a prefab part's box unticked and every copy losing that shadow;
Play agreeing; a scene saved before 049 casting as before; a new scene,
an added light unticked and an added shape ticked.
