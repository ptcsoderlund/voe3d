# 18 — A model dragged into a view is placed, or onto the Inspector swapped
folder: editor
decisions: 0168, 0277, 0202, 0204, 0217

## Change
Needs cards 15 and 16 (0277 point 8). Read the headers of `editor/src/entities.h`,
`editor/src/undo.h`, `editor/src/pick.h`, `editor/src/view.h`, `editor/src/inspector.h` and
`editor/src/assets_panel.h`.

- `editor/src/entities.h`, `editor/src/entities.c`: `[[nodiscard]] bool
  voe_editor_entities_model_add(voe_ecs_world *, const char *path, voe_math_double3 position,
  voe_ecs_entity *out);` — as `voe_editor_entities_add`, with the identity named after the
  path's last name without `.glb` by the same name rules, the transform at `position`, and a
  model row naming `path`, all queued; a failure leaves nothing, as the add's does.
- New `editor/src/assets_drag.h`, `editor/src/assets_drag.c`: what a held model row does.
  - A press on a model row (the rows' nodes, `assets_panel.h`) starts a drag holding its
    project-relative path; the row draws held while it lasts.
  - Released over a scene view: the view's pick ray through the pointer, `voe_3d_pick` with the
    store; the point is the hit, else where the ray crosses y = 0 in front of the eye, else 10 m
    along it; `voe_editor_entities_model_add`; the new thing is selected; one undo step and
    unsaved, marked the way Add entity marks them.
  - Released over the Inspector's rectangle while the selected thing has a model:
    `voe_3d_model_submit` with the new path; one undo step and unsaved, the way an Inspector
    edit marks them.
  - Released anywhere else, or with a press that started elsewhere: nothing.
  - Header points: the three outcomes, why the ground plane then 10 m, why the Inspector drop
    replaces the path rather than adding a component.
- `editor/src/main.c`: one call a frame beside `voe_editor_pick_read`, with the pointer, the
  button and the same `blocked`; a drag in progress blocks the pick's own press. Keep it under
  800 lines; move a block into `assets_drag.c` if it would not be.
- `editor/src/src.md`: lines for the new pair; the `entities` line names the model add.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0 and
`cmake -P check.cmake` exits 0. Dragging is the human's steps 4 and 5, card 19.
