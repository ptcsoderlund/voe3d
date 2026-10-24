# 10 — A layer's growth is set live and undone in one step
folder: editor
after: 09
decisions: 0168, 0473, 0476

## Change
0476 points 2 and 7's plumbing: an edit of how densely a layer grows a kind, asked by the Layers
section (card 11), set live through the store while held and one undo step when let go, as card
07's look edit is.

- `editor/src/models.h`, `editor/src/models.c` — new
  `voe_editor_models_grow_set(models, path, layer, kind, uint16_t density)` forwarding to
  `voe_3d_models_landscape_grow_set` (3d/models_foliage.h); a comment: tenths of copies per
  100 m², 0 not grown, saved with the layers.
- `editor/src/strokes.h`, `editor/src/strokes.c` — a grow stroke: `grow`, `layer`, `kind`,
  `grow_before`, `grow_after`; `voe_editor_stroke_new_grow(path, layer, kind, before, after)`;
  apply sets the density either way. Header and sketch follow.
- `editor/src/sculpt.h`, `editor/src/sculpt.c` — a grow ask and edit beside the look's:
  `grow_asked`, `grow_layer`, `grow_kind`, `grow_to`, `grow_held`, and the open edit's layer,
  kind, density before and path; settled in `voe_editor_sculpt_read` with the look edit, set
  live while held, one grow stroke when let go if the density changed. The default density a
  tick asks, 40 tenths (the brush's 4 per 100 m²), and the slider's range, 1 to
  VOE_ASSETS_LANDSCAPE_GROW_MAX, as constants. The header's paragraph on edits settled first
  names it.
- `editor/src/src.md` — the `models.*`, `strokes.*` and `sculpt.*` entries say growth.

## Done when
The editor builds, and `grep -n "voe_editor_stroke_new_grow" editor/src/sculpt.c` prints a line.
