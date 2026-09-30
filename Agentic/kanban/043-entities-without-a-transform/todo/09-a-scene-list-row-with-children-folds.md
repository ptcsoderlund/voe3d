# 09 — A Scene list row with children folds
folder: editor
after: 05, 08
decisions: 0168, 0300, 0302

## Change
0302 point 2: the identity's `folded` (card 02) is what the Scene list
shows. Read `scene/include/scene/identity_component.h`,
`scene/include/scene/identity_system.h`, `editor/src/scene.h`,
`editor/src/scene.c`, `editor/src/scene_list.h`, `editor/src/scene_list.c`
and `editor/src/src.md`.

- `editor/src/scene.h`: `voe_editor_scene_row` gains `fold`, the row's fold
  button as drawn, or `VOE_UI_NODE_NONE` for a row with no children.
- `editor/src/scene_list.c`: in the depth-first walk, a row whose entity
  has children gets a small button before its name showing folded or open
  in ASCII the face draws, keyed with the row so keys never change; a
  folded row's tree is not walked, so its rows are not listed. A selected
  entity inside a folded tree stays selected. A part's row folds too.
- `editor/src/scene.c`: where the rows are read after the frame, a fired
  `fold` submits the entity's identity row with `folded` flipped through
  `voe_scene_identity_submit` and counts one in `structural` (so it is an
  undo step and unsaved, as a drop is), or sets `full` when refused. A fold
  press neither selects nor starts a drag.
- `editor/src/scene.h`: the `structural` comment includes a fold.
- `editor/src/scene_list.h`: a paragraph: any row with children folds and
  opens, the state is the identity's and saved with the scene (0302), and
  folded rows are not listed.
- `editor/src/src.md`: the `scene_list.h` and `scene.c` entries.

## Done when
`grep -q folded editor/src/scene.c` exits 0, and the editor builds in the
folder's checks.

The human, in `examples/tank_game` (feature.md's How to test):
1. Add entity: a row, an Inspector with only the identity, nothing new in
   either view.
2. Add component → Transform on it: at the origin, with Remove.
3. A new entity given a shape gets a transform, no Remove on it; remove the
   shape and the transform has Remove.
4. A bare "Lamps" with three shaped things dragged under it: nothing moves;
   fold hides the rows, open shows them.
5. Folded, saved, closed and reopened: still folded.
6. Undo and redo each of the above.
7. Tank / Breakable on a bare entity, then Play: the game plays as before.
8. A scene saved before this change opens with everything in place.
