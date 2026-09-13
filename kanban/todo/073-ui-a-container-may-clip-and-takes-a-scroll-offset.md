# 073 — `ui`: a container may clip, and takes a scroll offset

claimed-by: -
blocked-by: 072
decision: *Overflow is opt-in: a container may wrap or clip, and a scroll area remembers its offset* (ADR-0153) points 5 and 6.

## Goal

A container can clip its descendants to its own rectangle on X, on Y or on both, and can be handed
an offset that moves its content. What is clipped away is neither drawn nor hit. Layout clamps the
offset and remembers nothing. A zeroed container behaves as today.

## Scope

**1. `ui/include/ui/layout.h`.**

```c
typedef enum {
	VOE_UI_OVERFLOW_VISIBLE = 0,   // today: children stick out
	VOE_UI_OVERFLOW_CLIP,
} voe_ui_overflow_kind;

typedef struct {
	voe_ui_overflow_kind x;
	voe_ui_overflow_kind y;
} voe_ui_overflow;

typedef struct {
	...
	bool wrap;
	voe_ui_overflow overflow;
	voe_math_float2 scroll;        // millimetres, on CLIP axes only
} voe_ui_container;

// The part of a node that can be seen: its rectangle intersected with every
// clipping ancestor's, axis by axis. Size nought on an axis when nothing is left.
voe_ui_rect voe_ui_node_visible(const voe_ui_context *ui, voe_ui_node node);

// The offset layout used for a container, after clamping.
voe_math_float2 voe_ui_node_scroll(const voe_ui_context *ui, voe_ui_node node);
```

Both accessors are readable in the same window as `voe_ui_node_rect` and refused in the same ways.

**2. Clipping, in layout.**
- A container that clips on an axis limits every descendant to its own rectangle on that axis —
  the whole rectangle, padding included. A VISIBLE axis does not narrow anything. Nested clips
  intersect.
- **Anchored descendants are clipped like any other** (ADR-0153 point 5).
- The root may clip; it is a container like any other.

**3. The offset, in layout.**
- A non-zero `scroll` on a VISIBLE axis is the caller's bug and asserts.
- **Clamp, per axis**, to between nought and `measured − arranged` on that axis, nought when
  that is negative. Do it where the container's own measure and rectangle are both known and its
  children are not yet placed, so a clamped offset is never a frame late.
- Every child of the container, in flow and anchored, is placed at minus the clamped offset on that
  axis; their descendants follow because they are placed from their parent's rectangle.
- `voe_ui_node_measured` of the container is not changed by its offset.

**4. Widgets use the visible rectangle, `ui/src/widgets.c`.**
- **Every element record's `clip`** is the rectangle it uses today intersected with the node's
  visible rectangle. **A record whose clip is empty on either axis is not emitted** and takes no
  element capacity.
- **Hit testing** tests `voe_ui_node_visible`, not the node's rectangle: a widget clipped out of
  sight is not hovered and cannot be armed or pressed.
- **A gesture already under way continues** when its widget scrolls or is clipped out of sight,
  exactly as it continues past the surface's edge today (`voe_ui_pointer.over`'s paragraph).

**5. Headers.**
- `layout.h`: an *OVERFLOW* paragraph beside *OVERFLOW IS NOT SHRUNK* — VISIBLE by default, CLIP
  per axis, clip at the rectangle with padding inside it, anchored children clipped, offset on
  CLIP axes only, clamped by layout, remembered by nobody here (a scroll area is card 075).
- `widgets.h` *WHAT IS NOT HERE*: *no clip narrower than a widget's own rectangle* is gone; say
  records are clipped to the visible rectangle and clipped-out widgets are not hit.

**6. Tests.**

`ui/tests/layout.c`:
- A 50×20 row clipping X holding a 70 box: the box's rect is 70 wide, its visible rect 50.
- Nested: a clip inside a clip visible-intersects on both axes; a VISIBLE-on-Y parent does not
  narrow Y.
- An anchored child placed past a clipping parent's edge has an empty visible rect.
- Offset: a 30-tall column clipping Y holding content 100 tall, `scroll.y = 25` → children 25
  higher, `voe_ui_node_scroll` 25; `scroll.y = 500` → 70; `scroll.y = −4` → 0; content 20 tall
  with `scroll.y = 10` → 0.
- An anchored child of a scrolled container moves with the offset.
- `scroll.x = 1` on a container visible on X asserts, if `voe::testing` can catch an assert;
  otherwise say so in Notes.

`ui/tests/widgets.c`:
- A button half clipped emits a record whose clip is the visible half; a label wholly clipped
  emits nothing and the element count says so.
- A pointer over the clipped-away half of a button does not hover it; over the visible half it does.
- A number box dragged and then scrolled out of view goes on reporting its drag.

## What must not change

- A zeroed container: every existing test passes unedited.
- No remembered offset, no scrollbar, no wheel, no key table — card 075.
- `render`: the element record's `clip` field already exists and is not touched.
- Nothing outside `ui/` except `ui/ui.md`.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R ui` passes.
- The editor runs and looks as before (no editor code changed).
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

`.overflow = { VOE_UI_OVERFLOW_CLIP, VOE_UI_OVERFLOW_CLIP }` on a panel keeps everything inside it
on screen and inside it for the pointer, and `.scroll` moves its content without ever running past
the content's end.

## Notes
