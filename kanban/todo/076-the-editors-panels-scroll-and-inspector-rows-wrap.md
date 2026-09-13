# 076 — the editor's panels scroll, and the inspector's rows wrap

claimed-by: -
blocked-by: 072, 074, 075
decision: *Overflow is opt-in: a container may wrap or clip, and a scroll area remembers its offset* (ADR-0153) point 11.

## Goal

Nothing in the editor draws outside its column. The inspector's field rows fold onto further lines
when the column is too narrow for a label and its number boxes. A column whose content is longer or
wider than it scrolls with the wheel or its scrollbar.

## Scope

**1. Every leaf that is not a scene view is a scroll area, `editor/src/dock.c`.**
- The leaf's panel keeps its background, size and padding, and takes `.across =
  VOE_UI_ACROSS_FILL`.
- Inside it, one `voe_ui_scroll_begin` keyed by the leaf's panel key and view, scrolling
  `{ .x = true, .y = true }`, sized grow along and filled across, holding what
  `voe_editor_panel_draw` draws today. The panel's gap moves onto the scroll area.
- Scene-view leaves are unchanged: no scroll area around a picture.

**2. The inspector's rows fill and wrap, `editor/src/inspector.c`.**
- The scroll area's content column and each component panel fill across (`VOE_UI_ACROSS_FILL`), so
  every field row is arranged at the column's width.
- Every row `field_row` and `rotation_rows` open takes `.wrap = true`, keeping its gap and its
  `across`. A narrow column puts the number boxes on lines beneath the label; a single box wider
  than the column is alone on its line and the area scrolls sideways to it.
- The Scene list (`editor/src/scene.c`) gets nothing but its leaf's scroll area.

**3. The wheel, `editor/src/main.c`.** Where the pointer is built:

```c
voe_platform_wheel wheel = voe_platform_input_wheel(window);
...
	.scroll = { wheel.x * WHEEL_MILLIMETRES, wheel.y * WHEEL_MILLIMETRES },
```

`WHEEL_MILLIMETRES` is a named constant of 10, with a line saying a notch's length is this
program's to choose and `ui` is handed millimetres. The scene views keep the middle button and read
no wheel.

**4. Capacities.** Wherever the editor's `voe_ui_context` is created, `scrolls` covers one per
non-picture leaf, with the arithmetic in a comment.

**5. `editor/editor.md`.** The *not built* list loses *no scrolling, no clipping*; one sentence
that each non-picture column scrolls and the inspector's rows wrap.

## What must not change

- The dock tree, its fractions and its seams.
- The scene views: their picture, their camera drag, and that a wheel over them does nothing.
- What the inspector edits, how a drag writes (ADR-0134, ADR-0136), selection in the Scene list.
- `ui`, `platform`, `render`: nothing but calls.

## Verify

- Linux: `cmake -P check.cmake` green.
- Linux, by hand, run the editor and select an entity with a transform:
  1. At the default window size, every number box is inside the Inspector column.
  2. Narrow the window until a vector field's boxes fold under its label; widen it and they return
     to one line.
  3. Narrow further until a single box is wider than the column: a horizontal bar appears and
     scrolling sideways reaches the box.
  4. Make the window short enough that the inspector overflows: the wheel scrolls it, the thumb drags
     it, a press on the track pages it, and dragging a number box still edits the value.
  5. Scroll to the bottom, then select an entity with fewer components: the column shows its top
     content with no empty frame.
  6. The wheel over a scene view does nothing.
- Save a screenshot of step 2 beside this card as `076-inspector-wraps.png`, and one of step 4 as
  `076-inspector-scrolls.png`.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

The inspector's numbers are on screen at any window width, folding under their labels when the
column is narrow and reachable by scrolling when a column is too small for them.

## Notes
