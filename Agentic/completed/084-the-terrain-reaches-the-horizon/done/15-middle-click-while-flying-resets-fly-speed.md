# 15 — A middle click while flying puts the fly speed back
folder: editor/src
after: none
decisions: 0168, 0233, 0396

## Change
Bug 01 of this feature: holding the right button (flying), a press of the
middle button must put the flying view's speed back to `FLY_METRES_PER_SECOND`,
the speed a new view starts at. A middle press while not flying is unchanged.
The owner is `voe_editor_views_fly`; fix it there, once.

- `editor/src/view.h`: `voe_editor_fly_keys` gains `bool middle`, the middle
  button's level this frame. `voe_editor_views_fly`'s contract gains: while a
  view flies, the middle button's down edge sets that view's speed back to the
  starting speed, before this frame's notches apply. The edge is read against
  `views->middle_was_down`, which `voe_editor_views_drag` writes; so the fly is
  called before the drag each frame (as frame_pointer.h's order already has
  it) — say so in the contract and on the `middle_was_down` field's comment.
- `editor/src/view.c`: in the fly's speed step (where `fly_speed` is scaled by
  `FLY_NOTCH`, around line 440), on `keys.middle && !views->middle_was_down`
  set the flying view's `fly_speed = FLY_METRES_PER_SECOND` before the notch
  scaling and clamp. The comment above the `FLY_*` rates names the middle
  press resetting the speed. The function that does the step may need `views`
  passed in, or the edge passed as a bool from `voe_editor_views_fly`; prefer
  the bool.
- `editor/src/frame_pointer.c`: where `voe_editor_views_fly` is called (around
  line 29–40), the keys gain `.middle = input->middle`. The middle drag
  (around line 214) is untouched.
- `editor/src/src.md`: the `view.h` entry names the middle press resetting the
  fly speed, within the entry's cap.

## Done when
The folder's checks build `voe_editor`, and
`grep -n 'keys.middle' editor/src/view.c` and
`grep -n '.middle = input->middle' editor/src/frame_pointer.c` both match.

Human, not the coder: in the editor, hold the right button over a view, wheel
up until flying is fast, press the middle button still holding the right, and
fly with WASD: the speed is the starting one. Middle drag without the right
button still orbits.
