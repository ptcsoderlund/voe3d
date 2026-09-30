# 10 — A picture dropped on the Inspector is the emitter's texture
folder: editor
after: 09
decisions: 0168, 0298

## Change
The feature's "a texture taken from the Assets panel". Read
`editor/src/assets_panel.h`, `editor/src/assets_panel.c`,
`editor/src/assets_drag.h`, `editor/src/assets_drag.c` and
`3d/include/3d/emitter_component.h`.

- `editor/src/assets_panel.h` and `.c`: `voe_editor_assets_row` gains
  `bool picture`, a file ending `.png`, `.jpg` or `.jpeg` in any case; drawn
  marked and held as a model row is (the panel's `held_prefab` sibling gains
  a `held_picture`, or whatever form the panel's held state takes). The
  header's row kinds name it.
- `editor/src/assets_drag.h` and `.c`: a held picture row released over the
  Inspector while the selected thing has an emitter replaces the emitter's
  texture through `voe_3d_emitter_submit` (the rest of the row as it is),
  one undo step and unsaved, the way the model drop marks them. Over a scene
  view, over an Inspector with no emitter, on a prefab's part, or anywhere
  else: nothing, and the ghost is drawn refused there. A path longer than
  the texture's room is refused too. `VOE_EDITOR_ASSETS_DRAG_PATH` covers
  `VOE_3D_EMITTER_TEXTURE`. The header's outcomes list gains the picture's.
- `editor/src/src.md`: the entries.

The human, after the build: open the tank game in the editor, add an
entity, then add Rendering / Particle emitter. Particles rise from it at
once. Change its speed and its colours in the Inspector, and the view
follows. Drag a `.png` from the Assets panel onto the Inspector, and the
particles take it. Ctrl+Z steps back through each change.

## Done when
`grep -q voe_3d_emitter_submit editor/src/assets_drag.c` exits 0, and the
editor builds in the folder's checks. The rest is the human's step above.
