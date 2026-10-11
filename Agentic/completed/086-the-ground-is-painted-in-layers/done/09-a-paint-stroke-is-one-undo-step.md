# 09 — A paint stroke is one undo step
folder: editor
after: 05
decisions: 0168, 0379, 0402

## Change
0402 points 3 and 7: the held drag paints when the brush is PAINT, and its release records the
rectangle's weights as one undo step. No Inspector control chooses PAINT yet (card 10).

- `editor/src/models.h`, `editor/src/models.c` — `voe_editor_models_put_paint(models, path,
  rect, const uint8_t *values)` over `voe_3d_models_landscape_put_paint`, and
  `voe_editor_models_layer_set(models, path, index, const char *material)` over
  `voe_3d_models_landscape_layer_set`; the header's landscape paragraph says paint and layers
  are edited here too and saved by Save with the heights (the writer carries them).
- `editor/src/sculpt.h`, `editor/src/sculpt.c` — `voe_editor_sculpt` gains `layer`, the layer a
  PAINT brush paints, and a press copy of the paint (`uint8_t *before_paint`, both planes of the
  grid, malloced at the press and freed at the release as `before` is). With kind PAINT the
  brush passed to `voe_editor_models_brush` carries `layer`; the press copies the paint, not
  the heights (flatten's target is not taken); the release builds a paint stroke. Escape and
  choosing none behave as for sculpting. The constraints line names the paint copy (8 bytes a
  point). If `sculpt.c` switches over the kind, PAINT joins it.
- `editor/src/strokes.h`, `editor/src/strokes.c` — a stroke is heights or paint: a `paint` flag
  and, for paint, the rect's bytes before and after as `voe_editor_models_put_paint` takes them;
  `voe_editor_stroke_new_paint(path, rect, before, after)` beside the heights constructor;
  `voe_editor_stroke_apply` puts the paint back for a paint stroke. Header: a stroke carries
  heights or weights, why both ride beside the text.
- `editor/src/undo.h` — its stroke paragraph says a stroke may be a paint stroke; `undo.c`
  needs no change if it only calls `voe_editor_stroke_apply` and `_destroy`.
- `editor/src/src.md` — entries for `sculpt.*`, `strokes.*`, `models.*` mention painting.

The editor has no tests; the folder's build is the check here and the human's steps at card 11.

## Done when
The editor builds, and `grep -n "put_paint" editor/src/strokes.c editor/src/models.c` prints a
line for each file.
