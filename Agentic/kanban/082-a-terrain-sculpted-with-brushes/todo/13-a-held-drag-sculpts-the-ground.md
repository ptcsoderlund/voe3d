# 13 — A held drag sculpts the ground
folder: editor
after: 04, 12
decisions: 0168, 0379

## Change
With a brush chosen and a landscape selected, the pointer finds the ground and a held left button
sculpts it, one undo step a stroke (0379 points 3–5).

- `editor/src/sculpt.h` / `sculpt.c` — the state gains the hover (`bool hit`, `float x, z` in the grid's
  own space, the entity) and the stroke (`bool stroking`, the last hit, flatten's `target`, the
  touched rect, a copy of the whole grid as it was at the press, its own memory freed at the stroke's
  end). New `voe_editor_sculpt_read(voe_editor_sculpt *, voe_editor_scene *, voe_editor_views *,
  voe_editor_models *, voe_editor_undo *, voe_editor_session *, voe_platform_pointer, bool left,
  bool over, float seconds, voe_base_arena *scratch)` → bool, true when it took the press this frame:
  - Active only while `chosen` and the selection `voe_editor_sculpt_wears` with a loaded entry; else
    clears the hover and takes nothing.
  - Hover: the view under the pointer's pick ray (as pick.c makes it) through `voe_3d_pick_landscape`
    (3d/pick.h, card 04).
  - Left down edge on a hit, over a view: the stroke starts, `target` the height there.
  - Held: stamps from the last hit to this one at most radius/4 apart, the frame's seconds shared,
    each through a new `voe_editor_models_brush` (models.h/.c, a thin call to
    `voe_3d_models_landscape_brush`), the rects joined; no hit this frame stamps nothing.
  - Release: a `voe_editor_stroke` made of the joined rect from the copy and the store (strokes.h),
    handed to `voe_editor_undo_stroke`, the project marked edited (`voe_editor_session_edited`); an
    empty rect records nothing.
- `editor/src/frame_pointer.h` / `frame_pointer.c` — the read runs after the middle drag and before the
  gizmo; while it takes the press or a stroke is held, the gizmo, the Assets drag and the pick are
  blocked. The order list in the header gains the step.
- `editor/src/main.c` — `voe_editor_models_settle` is called only while no stroke is held.
- `editor/src/src.md` — the sculpt and frame_pointer entries updated.

## Done when
`grep -c voe_editor_sculpt_read editor/src/frame_pointer.c` prints 1 or more, and the folder's check
passes.
Human: How to test steps 2 (the hill grows), 4, 5 and 7 in feature.md, on a thing whose model path is a
`.landscape` written in 0379's format.
