# 0148. A frame is a sequence of passes, and a target of one's own is a texture

- **Status:** Accepted
- **Date:** 2026-09-13
- **Deciders:** Human, Tech Lead
- **Supersedes:** —. Amends ADR-0051 only in that the window's target is now one target among
  several; everything it decided stands
- **Superseded by:** —

## Context

**Closes D-257**, opened by ADR-0147: *the shape of `render`'s API for more than one view per
frame, and how a view's image reaches a panel.* ADR-0147 decided that a scene view is a
camera's picture in its own target, shown on a panel, several of them, and that this is a
`render` capability every program gets.

Today `voe_render_frame_begin(device, size, view, light, &drawing)` takes **one camera and one
sun for everything drawn until `_end`**. The camera and sun are one block in one uniform
buffer per frame slot (`render/src/device_internal.h`, `struct voe_render_frame_block`), the
frame draws into one colour-and-depth pair per frame slot (ADR-0050, ADR-0051, `target.c`),
and `_end` blits that pair's colour image onto the window. Nineteen places call it, through
`voe_app_draw_open` or directly in tests.

**The evidence that settles the fork is already in the tree.** `editor/src/main.c` hands
`voe_app_draw_open` a zeroed view and a zeroed sun, because the editor draws no world into the
window — it draws only its interface, which has its own matrix. A camera attached to the whole
frame is a camera some programs have to invent.

Constraints already fixed. ADR-0050: frames in flight, so anything a frame writes is per frame
slot. ADR-0051: the frame is drawn offscreen and copied to the window. ADR-0069: the colour
target holds premultiplied linear colour. ADR-0135: `app` is parts a program calls, and it
hands out its device so a program can reach past it. `voe_render_capacities`: everything is
fixed at creation and a count that may be nought costs nothing.

## Options considered

### Option A — a frame is a sequence of passes, and the window is one target
`_begin` takes no camera. A pass opens onto a target with a camera and a sun, takes draws,
and closes. The window's own pair is a target like any other. Every caller changes by one
call, most of them inside `app`. Permanent: the shape of `render`'s main entry point.

### Option B — keep the frame's camera, add views drawn before it
No existing caller changes. The window's view stays special for ever, and a program drawing
no world into the window keeps inventing a camera for it — which is the zeroed camera the
editor passes today.

## Decision

**Option A**, the tech lead's recommendation, accepted by the principal with its details.

> *"I agree with your take"*

1. **`voe_render_frame_begin` takes no camera and no sun.** It waits for the slot, rebuilds
   what a resize invalidated, takes a swapchain image where there is a window, and opens the
   recording. `_end` still submits and blits the window's target.
2. **Draws happen inside a pass.** `voe_render_pass_begin(device, target, camera)` opens one
   onto a target; `voe_render_pass_end(device)` closes it. `camera` is a
   `const voe_render_pass_camera *` — the view and the sun together, which is the one block the
   shader reads — and **may be NULL for a pass that draws no mesh**: an interface pass has its
   own matrix per element range and no camera to invent, which is the whole of this
   decision's argument and must not survive into the pass by the back door. A mesh draw or a
   depth clear in a pass opened with no camera asserts.
   `voe_render_frame_draw`, `_draw_blended`, `_clear_depth` and `_draw_elements` assert that
   a pass is open. **Passes do not nest**; one is open at a time. Element *submission* stays a
   frame-wide buffer, so a range submitted once may be drawn in any pass of that frame.
3. **The window's target is a target, named by the zeroed id** — `VOE_RENDER_TARGET_WINDOW`.
   It is the frame slot's existing pair and nothing about its images changes.
4. **A target is cleared the first time a frame opens a pass onto it, and loaded after that.**
   Two passes onto one target in one frame draw the second over the first, with the depth left
   as the first pass left it. A target no pass opened in a frame keeps whatever it held — the
   window's is still cleared, because `_end` blits it.
5. **A target of one's own is created once, at a size, and resized.**
   `voe_render_target_create(device, width, height, &target, &texture)` makes a colour-and-depth
   pair **per frame slot** and hands back a target id and **a colour texture id that does not
   change**. Which slot's image that texture id reads is `render`'s bookkeeping: a frame in slot
   *n* samples the image slot *n* drew. `voe_render_target_resize` asks for a new size, which
   is applied at the top of the next frame, where the window's own rebuild already happens; the
   picture is undefined until a pass draws into it.
6. **The texture id is an ordinary one.** It goes wherever a colour texture id goes — a shading
   record's base colour slot, which is how a panel standing in the world shows a view — and into
   a new element kind (point 7). **Sampling a target inside a pass drawing into that same target
   is the caller's bug and asserts.** Sampling it in a later pass of the same frame, or a later
   frame, is what it is for.
7. **A third element kind, `VOE_RENDER_ELEMENT_IMAGE`**, samples `sheet_texture` over the
   element's bounds through its `sheet` rectangle and multiplies by its colour. The record's
   size does not change; this is what `kind` and `sheet_texture` were shaped to allow. A view on
   a flat panel, an icon and a thumbnail are all this.
8. **Two capacities join `voe_render_capacities`**: `targets`, how many targets of one's own the
   device holds, which may be nought; and `passes`, how many passes one frame may open, per frame
   slot, which is at least one. Asking for one more is a returned failure at the call that asked,
   as every other capacity is.
9. **`app` loses the camera and the sun from `voe_app_draw_open`** and gains nothing: a program
   opens its passes on `voe_app_device(app)`, which is the reach-past ADR-0135 already provides.
   **`3d`'s draw system draws into the open pass** and asserts one is open.
10. **A view nobody can see is not drawn.** That is the caller's to honour by not opening its
    pass; `render` has no notion of visibility and gains none.

## Blast radius

**Moderate.** Every caller of `_begin` is touched once, and `render`'s own tests with them. The
permanent part is point 1 — the frame no longer owns a camera — and point 5's promise that a
target's texture id is stable across resizes and frames in flight, which panels and shading
records will come to depend on. Going back to Option B would re-attach a camera to the frame and
leave the passes as the special case.
Reversibility: **moderate.**

## Consequences

- **The editor stops passing a zeroed camera.** It opens a pass onto the window for its
  interface and one pass per visible scene view.
- **`dev` gains one call** — a pass onto the window around what it draws today — and its picture
  does not change.
- **Resizing a view costs a wait**, the way resizing the window already does: the in-flight
  slots' images are in use. A splitter dragged continuously is a resize per frame; if that
  measures badly, the answer is a target kept at a larger size and drawn into partly, and it is
  a later card.
- **The uniform block becomes per pass.** One camera-and-sun block per pass per frame slot, bound
  per pass; that is `render`'s internal change and is what `passes` counts.
- **Post-processing gets a natural home later**: a pass reading one target's texture and writing
  another is what a tone-mapping pass is. Nothing here builds it.
- **Nothing here decides a view's *way of drawing*** — debug channels, baking information. That
  is D-258, parked.

## Rejected options and why

**B — keep the frame's camera.** It avoids touching callers once and costs a special case for
ever, and the special case's cost is already visible: the editor's zeroed camera.

**A target that a frame asks for by size, instead of one created and kept.** A panel needs a
texture id that is the same next frame; an id that lives one frame would have every panel
re-resolving its picture every frame. Rejected with the details the principal accepted.

## Questions this opens

None new. D-258 (a view's way of drawing) already holds the question this would otherwise open.
