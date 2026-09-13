# 075 — `ui` has a scroll area

claimed-by: -
blocked-by: 073
decision: *Overflow is opt-in: a container may wrap or clip, and a scroll area remembers its offset* (ADR-0153) points 7–9.

## Goal

A widget that clips its content, remembers how far it is scrolled, draws a scrollbar over the
content, and is scrolled by the pointer's wheel, by a thumb drag and by a press on the track. The
wheel arrives as a length in millimetres, the form a stick or a hand will use too.

## Scope

**1. `ui/include/ui/layout.h`.** `voe_ui_capacities` gains `uint32_t scrolls` — how many scroll
areas one frame may hold. Nought is allowed and means none.

**2. `ui/include/ui/widgets.h`.**

```c
typedef struct { bool x; bool y; } voe_ui_scroll_axes;

// A column that clips on both axes and scrolls on the axes named. Its
// `overflow` and `scroll` are the area's own: a caller who set either asserts.
// Closed by voe_ui_end.
voe_ui_node voe_ui_scroll_begin(voe_ui_context *ui, const char *name,
				uint32_t index, voe_ui_container container,
				voe_ui_scroll_axes axes);
```

**Not on this card, by rule 10:** `voe_ui_scroll_by` and `voe_ui_scroll_reveal` (ADR-0153 point 9)
have no caller until gamepad focus exists. Write section 4 so the pointer's length goes through one
internal function that takes a starting area and a length, which `scroll_by` will later expose.

`voe_ui_pointer` gains `voe_math_float2 scroll` — millimetres this frame, positive showing content
further right or down; nought is no scroll, so every existing initialiser keeps its meaning.

**3. What is remembered.** A table in the context, `scrolls` long, of widget key → offset.
- `voe_ui_scroll_begin` claims its key like every widget and hands layout the remembered offset
  (nought for a key not in the table). **After layout, the table stores `voe_ui_node_scroll`** —
  the clamped offset.
- **An entry whose area was not called this frame is dropped at frame end.**
- A frame with more scroll areas than `scrolls` is refused exactly as too many nodes is: named on
  stderr, `voe_ui_frame_end` false.
- An axis not in `axes` is clipped and its offset is always nought.

**4. Scrolling by a length, and passing it on.** The innermost area takes as much of each axis as its clamp allows; **the remainder goes to the next
scroll area outward**, and so on; what nobody can take is dropped.
- The pointer's `scroll` starts at the innermost scroll area whose visible rectangle holds
  `pointer.at`, and only while `pointer.over`. Worked out inside `voe_ui_frame_end`, after the hit
  test, against this frame's rectangles, and lands in the next frame's layout.

**5. The scrollbar, over the content.**
- Drawn after the area's children, inside the area's rectangle: a vertical bar along the right
  edge, a horizontal one along the bottom, each **only when its axis scrolls and has range**
  (measured greater than arranged). When both show, each stops short of the corner by the other's
  thickness. Takes no layout space.
- **Track** and **thumb** are solid records clipped to the area's visible rectangle. Thumb length
  along the track is `track × arranged / measured`, never less than a named minimum; its position is
  proportional to the offset.
- Constants in `widgets.c` beside the button's, until the theme card: thickness 1.5 mm, minimum
  thumb 5 mm, track colour, thumb normal, hovered and held.
- **The bar is in front of the content for the pointer.** Pressing the thumb holds it; moving the
  pointer by d along the track moves the offset by `d × measured / arranged`; a gesture under way
  continues off the bar and off the surface. **A press on the track outside the thumb** moves the
  offset one arranged length towards the pointer, once per press, and holds nothing.

**6. Header paragraphs**, `widgets.h`: why the offset is remembered here and not in layout; that a
disappearing area forgets; the pass-outward rule; that a scroll arrives as millimetres so a stick or a
hand can produce it; that it lands next frame and why; that the bar overlays content, so pad the
area for clearance. *WHAT IS NOT HERE*: no scrolling by a program or to a node yet (they arrive with
focus), no dragging the content itself, no focus, no smooth scrolling, no saved offset.

**7. `ui/tests/widgets.c`**, headless as the file already is.
- A 30-tall area over content 100 tall: pointer `scroll.y = 10` → next frame's children 10 higher;
  `scroll.y = 500` → 70.
- Forgetting: an area skipped for one frame starts at nought when it returns.
- Passing on: an inner area at its end hands the rest to the outer; an inner area with range takes
  it all and the outer is unmoved; an inner area scrolling Y only passes X outward whole.
- Bar: no record when content fits; thumb length and position at offsets 0, 35 and 70; a thumb drag
  of 3 mm moves the offset by 10; a track press below the thumb moves by 30 once; a hovered thumb
  hides the button beneath it from the pointer.
- Refusal: `scrolls = 1` and two areas → `voe_ui_frame_end` false.
- Every test that predates this card passes unedited; a zero `scrolls` is allowed, so none needs it.

## What must not change

- `layout.c`'s rules from cards 072 and 073, except reading the capacity.
- `platform`, `editor`, `render`. No wheel reading here — the caller converts to millimetres.
- Card 034's hovered and held behaviour for every widget not under a bar.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R ui` passes.
- The editor runs and looks as before.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

Content taller than its area scrolls under the wheel, the bar tracks it and can be dragged or
paged, and a nested area hands on what it cannot take.

## Notes
