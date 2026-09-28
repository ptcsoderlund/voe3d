# 14 — The editor's views draw, pick and outline models
folder: editor
decisions: 0168, 0277, 0202, 0203

## Change
Needs cards 10 and 11 (the editor does not build until this card: `voe_3d_pick` changed).
Nothing loads a model yet; that is card 15. Keep `editor/src/main.c` additions to a few lines:
it is 750 and must stay under 800.

- New `editor/src/models.h`, `editor/src/models.c`: `voe_editor_models` owning one
  `voe_3d_models` store; `voe_editor_models_new`/`_destroy` (clears through the device, then
  destroys), and `const voe_3d_models *voe_editor_models_store(const voe_editor_models *)`.
  Header: the editor's one store, why one for the program and not per project (the device
  outlives projects; card 15 empties it on a swap).
- `editor/src/main.c`: make it after the shapes upload, destroy it before the device; pass the
  store to the pick read and to both view-pass calls.
- `editor/src/pick.h`, `editor/src/pick.c`: `voe_editor_pick_read` takes `const voe_3d_models
  *models` after `geometries` and hands it to `voe_3d_pick`; the header says models are picked.
- `editor/src/view_passes.h`, `editor/src/view_passes.c`: `_preview` and `_draw` take `const
  voe_3d_models *models`; each frame they build sets `frame.models` and its
  `outlined.models`. `VOE_EDITOR_CAPACITIES` adds `VOE_3D_MODELS_VERTICES`, `_INDICES`,
  `_GEOMETRIES`, `_SHADINGS` to the shapes' numbers and counts objects with
  `2 * VOE_GAME_WORLD_MAX_DRAWN` where it had `VOE_GAME_WORLD_MAX_DRAWN`; the comment says a
  model part is an object.
- `editor/src/src.md`: a `models` pair of lines; the `pick` and `view_passes` lines say models.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/coin_game --capture "$d/c.png" &&
test -s "$d/c.png"` exits 0.
