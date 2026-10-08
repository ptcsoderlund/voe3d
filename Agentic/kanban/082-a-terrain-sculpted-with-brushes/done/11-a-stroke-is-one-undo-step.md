# 11 — A stroke is one undo step
folder: editor
after: 10
decisions: 0168, 0204, 0379

## Change
The undo line carries a stroke beside a state's scene text (0379 point 5). Card 13 records them.

- New `editor/src/strokes.h` / `strokes.c` — `voe_editor_stroke`: `char path[VOE_3D_MODEL_PATH]`,
  `voe_3d_landscape_rect rect`, `float *before`, `float *after` (the rect's heights, row-major).
  `voe_editor_stroke *voe_editor_stroke_new(const char *path, voe_3d_landscape_rect rect,
  const float *before, const float *after)` copies both; `voe_editor_stroke_destroy` (NULL is nothing);
  `voe_editor_stroke_apply(const voe_editor_stroke *, voe_editor_models *, bool forward)` writes `after`
  or `before` through a new `voe_editor_models_put` (models.h/.c, a thin call to
  `voe_3d_models_landscape_put`). Header: why a stroke rides beside the text and is its own memory
  (rule 11's long-lived exception: a line of 64 may hold megabytes).
- `editor/src/undo.h` / `undo.c`:
  - `voe_editor_undo_state` gains `voe_editor_stroke *stroke` (NULL for none).
  - New `void voe_editor_undo_stroke(voe_editor_undo *, voe_editor_project *, voe_base_arena *scratch,
    voe_editor_stroke *)` — takes the stroke: records the scene first when the line is empty, then
    pushes a state with the text at `at` and this stroke, throwing away what could be redone, as a
    differing settle does.
  - `voe_editor_undo_take` gains `voe_editor_models *models`: back from a state with a stroke applies
    its `before`; forward onto one applies its `after`; the text is read as ever.
  - Every state dropped — the oldest shifted out, redo thrown away, `_forget`, `_restore` dropping the
    prefab's line — destroys its stroke. `_aside` keeps the level's.
  - Header: a step may be a stroke whose text equals the one before it, and the cost in memory.
- `editor/src/frame_commands.c` — the take's call passes the models.
- `editor/src/src.md` — the strokes entries; the undo entries name the stroke.

## Done when
`grep -c voe_editor_stroke_apply editor/src/undo.c` prints 1 or more, and the folder's check passes.
