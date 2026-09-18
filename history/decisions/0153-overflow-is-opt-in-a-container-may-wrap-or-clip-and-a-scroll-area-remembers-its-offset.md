# 0153. Overflow is opt-in: a container may wrap or clip, and a scroll area remembers its offset

- **Status:** Accepted
- **Date:** 2026-09-13
- **Deciders:** Human, Tech Lead
- **Supersedes:** in part — ADR-0095's *What overflow does* and its *No wrapping* consequence;
  ADR-0142 point 6's *scrolling and clipping* for the editor
- **Superseded by:** —

## Context

**Closes D-155** (whether wrapping is added, and when), **the transient half of D-141** (where
a remembered widget value lives) and **the default half of D-185** (whether an anchored child
escapes its parent's clip), the last opened in ADR-0102's text and never given a row.

The editor opened on three columns and the inspector's number boxes run off the right edge of
the window. Nothing is wrong in any one place. The Inspector column is a fixed fifth of the
window (`editor/src/dock.c`), a vector field is a row of a label and three number boxes at their
text's width (`editor/src/inspector.c`, `field_row`), layout reports overflow and does not shrink
it (ADR-0095), and every element is clipped to its own rectangle and nothing narrower
(`ui/include/ui/widgets.h`). The right-hand column sticks out past the window. D-155's trigger,
*the first interface that is genuinely too narrow*, is this.

The principal, asked whether overflow was planned:

> *"We are doing flexbox inspired, so we want the same behaviour as flexbox. But options to
> restrict or turn off automatic layout changes. Just like html/css but more lightweight and
> straightforward. So yeah, new row when overflow unless its one child taking over all space,
> then we do scrollbars. But we should be able to turn that off. […] gui should cover our
> editor, in vr and in games. So it should work with gamepads only as well as mouse+keyboard,
> vr hands and maybe more."*

And on the defaults:

> *"CSS defaults: no wrap. I think we need to require devs to be explicit and do things
> deliberately. Our philosophy is opt-in and as much as possible is off by default."*

Fixed constraints:

- **ADR-0091** — immediate mode. **ADR-0093** — `ui` is given its input as values and names no
  `platform`. **ADR-0092** — every element record carries its own clip rectangle, so a clip costs
  no draw-call break.
- **ADR-0095** — a lighter flexbox: natural, fixed or grow along; `along` and `across`; no shrink,
  no margin. Its amendments stand.
- **ADR-0102** — anchored children, measured against the parent's content box.
- **ADR-0141** — the interface is world geometry in millimetres; a VR build replaces the camera
  and the pointer source and nothing else.
- **`ui/include/ui/layout.h`** — layout keeps no state and has no identity; identity is
  `widgets.h`'s. Nothing is laid out until `voe_ui_frame_end`.

## Options considered

### Option A — clip every panel, add a vertical scroll area, nothing else
Nothing draws outside its column. A value wider than the column is cut off and unreachable, since
no editor scrolls an inspector sideways.

### Option B — fit the inspector to its width, in `editor/` only
Number boxes grow to share the row and the label moves above them when narrow. Cheapest, and
what Godot, Unity and Blender show. Solves one panel; every game and VR interface solves it again.

### Option C — flexbox's wrap and overflow in `ui`, off unless asked for
A container may wrap its run onto further lines and may clip on either axis; a scroll area is a
widget that remembers an offset. A zeroed container behaves exactly as today.

### Option C′ — the same, on by default
Rows wrap and columns scroll unless told not to, so nothing is ever lost off an edge.

## Decision

**Option C**, against the tech lead's first recommendation of B. The deciding factor is the
principal's: one interface system serves the editor, games and a headset, and a behaviour that
changes a layout is something a developer asks for.

1. **A zeroed container behaves exactly as today.** No wrapping, and children that do not fit
   stick out past the container, as CSS's `nowrap` and `overflow: visible`. Every existing call
   site keeps its meaning without an edit.
2. **`wrap` is one switch on a container and it wraps along the flow.** A row wraps onto further
   rows below; a column onto further columns to the right. One `gap` separates children and lines
   alike.
   - **A child starts a new line when it would not fit on the current one.** The first child on
     a line never breaks, so **a child longer than the whole line has that line to itself at its
     full size** — and sticks out, is clipped or is scrolled to, as the container's overflow says.
     That is the principal's *unless it's one child taking over all space*.
   - **Grow children share what is left of their own line.** `along` distributes within each line.
   - **A line is as thick as its thickest child**, and `across` places each child within its line.
     **Space left over across the container is shared equally between the lines**, which is CSS's
     default `align-content: stretch` — so a container whose children fit on one line is laid out
     exactly as it is today, FILL included.
   - **A wrapping container wraps against the length it was arranged at.** A fixed, grow or
     parent-filled length wraps; a natural one is as long as its single line and so never wraps.
     Not an error, and the header says so.
3. **Layout resolves X for the whole tree, then Y.** Measure and arrange X, then measure and
   arrange Y. A wrapping row breaks its lines while X is arranged, so its height is known when Y is
   measured. Nothing laid out today needs a height to know a width, so every existing tree lays out
   identically.
   - **The consequence this buys: a wrapping column breaks while Y is arranged, after its width
     was settled at one column's width.** Its extra columns stick out to the right of its own
     rectangle and are handled by its overflow. A column that grows wider as it wraps would need Y
     before X, and is left for the first interface that wants one (D-264).
4. **`voe_ui_node_measured` of a wrapping container is its content after wrapping**: the longest
   line along the flow, and the lines and their gaps across it, plus padding. Measured greater than
   arranged still means *did not fit*, and it is what a scroll range is computed from.
5. **Overflow is set per axis, named X and Y, and is VISIBLE or CLIP.** Absolute axes, as padding
   and anchors are, because a clip is about the container's own box and not its flow.
   - **CLIP limits every descendant to the container's rectangle** — padding inside the clip, as
     CSS clips at the padding box. Nested clips intersect. It reaches the element records as their
     clip rectangle.
   - **A pointer outside a clipping ancestor's visible rectangle hits nothing inside it.** A number
     box scrolled out of sight cannot be hovered, pressed or dragged.
   - **An anchored child is clipped like any other child** — the D-185 default. A child that must
     escape its parent's clip, a dropdown or a tooltip, is the first of those to decide (D-185 stays
     open for the escape).
6. **Layout takes a scroll offset, clamps it, and keeps no state.** A container carries an offset
   in millimetres, on a CLIP axis only, that moves all its children, anchored ones included, by
   minus that offset. **Layout clamps it** to between nought and measured less arranged, because
   layout is where both are known before the children are placed — so content that shrinks is never
   shown scrolled past its end, not even for a frame — and reports the offset it used. It remembers
   nothing. An anchored child that must stay put while content scrolls is anchored to a
   non-scrolling parent instead.
7. **The scroll area is a widget.** `voe_ui_scroll_begin(ui, name, index, container, axes)` opens a
   container that clips on both axes and scrolls on the axes named, with its offset remembered under
   its widget key.
   - **Remembered offsets live in a table in the context**, its capacity given at creation beside
     nodes and elements. **An entry whose scroll area was not called in a frame is dropped at that
     frame's end** — a scroll area that disappears and comes back starts at the top. This spends
     D-141's transient half. **Saving an offset into a scene stays parked** with D-141's other half.
   - **What it remembers is the offset layout used**, clamped (point 6), so a remembered offset is
     always one the content allowed.
8. **The scrollbar sits over the content.** It is drawn after the area's children, inside the
   area's rectangle along its far edge, only on an axis that has something to scroll, and it takes
   no space from layout — so a bar appearing never re-wraps anything. Its thickness and colours are
   constants until the theme card. A caller who wants content clear of it pads the area.
   - **Dragging the thumb** moves the offset in proportion. **A press on the track** moves one
     visible length towards the pointer, once per press.
9. **Scrolling arrives in two forms that any input device can produce, and neither is the mouse.**
   - **Scroll by a length**: millimetres on each axis, positive showing content further right or
     further down. The pointer's is a field on `voe_ui_pointer`, routed to the innermost scroll area
     under the pointer; a program with a stick or a hand calls `voe_ui_scroll_by` on a scroll area it
     names. **Whatever an area cannot take — it is at its end on that axis — passes to the next
     scroll area outward.**
   - **Reveal a node**: `voe_ui_scroll_reveal(ui, node)` after `voe_ui_frame_end` moves every scroll
     area around that node, innermost first, by the least that brings the node's rectangle into view,
     or its near edge where it is larger than the view. **This is the call gamepad focus will make**,
     and how focus moves is the next decision (D-262).
   - **Both take effect in the next frame's layout.** The rectangles a frame was hit tested against
     are the ones the person saw, which is why a change never lands mid-frame.
   - **The two calls are built with their first caller**, the card that follows the focus decision
     (rule 10). The pointer's scroll is built now, through the one internal path they will expose.
10. **The wheel reaches `platform` in notches.** `voe_platform_input_wheel` beside
    `voe_platform_input_motion`, drained by the poll, X and Y, fractional where a device reports
    fractions, positive where content moves further right or down. How many millimetres a notch is
    is the program's, as the pointer's millimetres already are.
11. **The editor opts in.** Each dock leaf that is not a scene view is a scroll area on both axes,
    so nothing leaves its column; the inspector's field rows wrap; the inspector's component panels
    and field rows fill the column's width, so there is a length to wrap against.

## Blast radius

**Moderate.** Three parts, priced separately.

- **The X-then-Y pass is the expensive one to reverse.** It is the shape of `layout.c`'s heart,
  and everything later measured against a width — word-wrapped text most of all — will be written
  on it. It is also the shape that makes word wrap possible at all, which is why taking it now is
  cheaper than later.
- **The new fields and calls are additive**, and zeroed means today; no call site changes.
- **Off by default is a policy and cheap to reverse**: flipping a default later is one line and a
  sweep of the call sites that relied on it.

Reversibility: **moderate** for the pass, **cheap** for the rest.

## Consequences

- **Every existing `ui` and editor test must pass unchanged** after the two-pass rewrite. That is
  the rewrite's verification and the reason to take it on its own card.
- **A developer who wants content never lost must say so twice**: `wrap` and a scroll area. The
  principal's philosophy chose this; the cost is that a forgotten opt-in looks like today's bug.
- **The consequence I do not like: a wrapping column does not widen.** Its extra columns overflow its
  own rectangle. It is the honest result of X before Y, and it is stated in the header rather than
  discovered.
- **One frame between a wheel notch and the content moving.** Invisible at the frame rates the
  engine runs, and the price of hit testing against what was seen.
- **Anchored children scroll with the content.** A pinned header over a scrolling list is two
  containers.
- **Scroll offsets are forgotten when an area is not drawn**, so switching the inspector to another
  entity and back starts it at the top. That is the transient half on purpose.
- **The scroll area's thumb is a solid element**; D-168's new shape kinds are not needed for it.
- **Card 035's brief loses its scroll area** and keeps the checkbox and the slider.

## Rejected options and why

**A — clip and scroll only.** Fixes where things draw, not what can be reached. A value wider than
its column needs either wrapping or a sideways scrollbar, and without wrap it is always the
scrollbar.

**B — fit the inspector in `editor/`.** Solves the one panel on the screen and none of the
interfaces the principal named. It remains available to the editor later as a styling choice on
top of C.

**C′ — on by default.** Content never lost off an edge, and a layout that rearranges itself
without being asked. The principal's rule: behaviour is opt-in, and a developer writes what they
mean.

**A scrollbar that takes space.** A bar appearing narrows the content, which re-wraps and can change
the height that made the bar appear: a second pass with a loop to bound. Over the content there is
no loop, and the Windows 10 target draws its bars that way.

**A reserved gutter.** No loop, and width lost in every scroll area whether or not anything scrolls.

**Shrink.** CSS has it and ADR-0095 refused it. Wrapping and scrolling cover what shrink is reached
for, and the principal asked for wrap and scroll, not shrink.

**Scroll state in layout.** Layout has no identity and keeps nothing; a remembered value needs a key,
and the key is the widget's. Layout takes the offset as a value like any other.

**A wheel-shaped API.** A scroll area driven by wheel events cannot be driven by a thumbstick, a
hand or a focus move without a second path. Two modality-free calls serve all of them.

## Questions this opens

- **D-262 — how focus moves between widgets without a pointer**: a gamepad's direction or a
  keyboard's Tab. Spatial nearest-in-direction, declared order, or both; what is focusable; how focus
  is drawn; how it enters and leaves a scroll area. Hot, and the next decision.
- **D-263 — dragging content to scroll**, as a touch screen and a VR hand expect, rather than the
  thumb. Parked until the first touch or hand input.
- **D-264 — a wrapping column that widens as it wraps.** Parked until the first interface that
  wants one.
- **D-265 — word wrap inside a label.** `wrap` moves children between lines, never words within a
  label. It is `text`'s measurement against a width, which point 3's pass makes possible. Parked
  until the first label longer than the space it is given.
