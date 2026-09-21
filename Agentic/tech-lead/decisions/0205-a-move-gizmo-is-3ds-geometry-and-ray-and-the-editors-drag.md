# 0205 — A move gizmo is `3d`'s geometry and its ray, and the editor's drag alone

date: 2026-09-21
by: planner

## Decision
The move gizmo is built where the selection outline is built. `3d` gains `gizmo.h`: the gizmo's size in metres
worked out from one camera so it covers the same pixels at any distance, the triangles of its three arrows,
three plane squares and three labels as this frame's geometry, which handle a ray meets, and where on that
handle's axis or plane the ray lands. All of it is arithmetic over one position and one camera, testable with
no graphics card, and none of it knows what a selection, a pointer or a theme is.

It is drawn by `voe_3d_draw_system_run`, from a `voe_3d_frame` field beside `outlined`, after the outline and
behind a second `voe_render_frame_clear_depth`, so it stands in front of everything in the picture including
the outline. Two draws and two transient ranges: the handles at rest in one colour and the marked handle in
another, because a drawn object's colour is one record per draw (ADR-0191). The record they wear is the
shapes' unlit outline material (0203); nothing is added to `render` and no fourth pipeline exists.

The editor owns everything a person does: which handle is under the pointer, the press that captures one, the
travel turned into a position, and the `voe_scene_transform_intent` that moves the entity. It reads the
primary button only — the middle button is the views' camera (`editor/src/pick.h`) — and a press it takes is a
press `pick.h` does not see, so a click on a handle never changes the selection.

## Reasoning
- The draw has to happen inside the view's pass, after the depth clear, and `3d/draw_system.c` is the only
  place that knows where that moment is. An editor that drew the gizmo itself would either draw it in the
  wrong order or need `render` to expose the ordering.
- The geometry and the ray are one question asked twice — what is drawn must be what is hit — so they belong
  in one module with one set of constants. Splitting them across two folders is how a gizmo comes to be
  half a centimetre off the thing you can grab.
- `main.c` is a call site and everything in it is wiring (its own header): cone and quad arithmetic in the
  editor would be code with no test and no folder.
- Rejected: a picking pass that reads the handle back off the card — a readback per frame and a second target,
  for a question the CPU answers in a few dozen multiplications over six handles.
- Rejected: entities in the world for the handles — they would be in `Scene`, in the Inspector and one day in
  the saved file, which is what ADR-0125 already refuses for a view's camera.

## Replaces
Nothing. It follows 0202 for the ray and 0203 for how this frame's geometry reaches a pass.
