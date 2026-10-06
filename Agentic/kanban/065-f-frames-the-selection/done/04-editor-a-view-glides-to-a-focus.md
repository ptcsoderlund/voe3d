# 04 — a view glides to a focus and distance
folder: editor/src
after: 03
decisions: 0168, 0371

## Change
A scene view can be sent to a new focus and distance and moves there over a quarter second,
keeping its yaw and pitch (decision 0371 point 3). Nothing calls it yet but the per-frame step.

- `editor/src/view.h` — `voe_editor_view` gains the glide's state: whether it is gliding, the
  focus and distance it started from and is going to (focus double), and the seconds gone. New:
  - `void voe_editor_view_glide_to(voe_editor_view *view, voe_math_double3 focus,
    float distance)` — starts a glide from where the view is now; the distance clamped to the
    orbit's closest.
  - `void voe_editor_views_glide(voe_editor_views *views, float seconds)` — once a frame, every
    gliding view moves by smoothstep of the time gone over `VOE_EDITOR_VIEW_GLIDE_SECONDS` (0.25),
    the focus lerped in double, its eye put back by the orbit; at the end it lands exactly on the
    target and stops gliding.
  Header points to add: the glide is the third way a view moves, never saved; a drag or fly
  starting on a view ends its glide where it is; the views opening on the camera ends all.
- `editor/src/view.c` — the two; `voe_editor_views_drag` and `voe_editor_views_fly` clear the
  captured or flying view's glide on their down edge; `voe_editor_views_focus_camera` clears every
  glide.
- `editor/src/frame_pointer.c` — call `voe_editor_views_glide` with the frame's seconds just
  before the fly.
- `editor/src/src.md` — the `view.h` and `view.c` entries name the glide.

## Done when
- `cmake --build --preset debug --target voe_editor` exits 0.
- `grep -n "voe_editor_views_glide" editor/src/frame_pointer.c` finds the call.
