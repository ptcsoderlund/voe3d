# 10 — The editor writes sculpted heights in the frame and has room for the nodes
folder: editor
after: 09
decisions: 0168, 0386, 0396

## Change
Card 06 removed the landscape settle and the transient chunk capacities from `3d/models.h` and
changed `voe_3d_models_landscape_frame` to `(models, device)`, written inside the frame before its
first pass, within `VOE_3D_LANDSCAPE_WRITE_TEXELS` a frame (0396 point 5).

- `editor/src/models.h` and `editor/src/models.c`: `voe_editor_models_frame(models, device)`
  calls the new frame; its comment says dirty heights are written into the landscape's texture,
  the rest carried. Remove `voe_editor_models_settle`.
- `editor/src/main.c`: the settle call and its comment go (around line 624); the frame call
  (around 635) loses `scratch` and its comment says heights, not chunks. If
  `scene.sculpt.stroking` was read only for the settle, leave the field; it is the sculpt's.
- `editor/src/view_passes.h`, `VOE_EDITOR_CAPACITIES` and the comment above it: the three
  transient numbers lose the `VOE_3D_LANDSCAPE_TRANSIENT_*` terms and that sentence;
  `objects` adds `VOE_3D_LANDSCAPES_DRAWN × VOE_3D_LANDSCAPE_NODES` per pass that draws the
  world, counted as the world's drawn objects are; `heights_texels` is
  `VOE_3D_LANDSCAPE_WRITE_TEXELS`.
- `editor/src/src.md`: the models entry no longer says settle.

## Done when
`grep -rn 'LANDSCAPE_TRANSIENT\|models_settle\|landscape_settle' editor` finds nothing, and the
folder's checks build `voe_editor`.
