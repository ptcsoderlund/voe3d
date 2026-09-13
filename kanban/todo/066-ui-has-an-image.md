# 066 — `ui` has an image

claimed-by: -
blocked-by: 065
decision: *A frame is a sequence of passes, and a target of one's own is a texture* (ADR-0148) point 7 — a picture on a flat panel is an IMAGE element; this card lets `ui` emit one.

## Goal

A `ui` caller places a picture — any colour texture id, including a target's — as a node in the
layout, sized like any other node, and it comes out as one `VOE_RENDER_ELEMENT_IMAGE` record.

## Scope

**1. `ui/include/ui/widgets.h`.**

```c
voe_ui_node voe_ui_image(voe_ui_context *ui, const char *name,
			 voe_render_texture texture, voe_math_float4 sheet,
			 voe_ui_size width, voe_ui_size height);
```

- Match the argument order and sizing idiom of the widgets already in the header if this sketch
  differs from them (how `voe_ui_panel_begin` takes its size, whether a name is taken).
- `sheet` is the part of the texture shown, in texture coordinates, `xy` corner and `zw` size —
  the element record's own shape. Whole picture is `{ 0, 0, 1, 1 }`.
- It emits **one** element: kind IMAGE, bounds the node's laid-out rectangle, clip as the other
  widgets clip, colour opaque white, `sheet_texture` the texture id's index half.
- It is a leaf: no children, no hover, no click. **A caller that wants to know whether the
  pointer is over the picture asks the node**, the way it would ask a panel; if the header has no
  such query for a non-button node, say so in Notes rather than adding one.
- Header paragraph: what it is for (a view, an icon, a thumbnail), that it counts as one element
  against the frame's budget, and that the texture's lifetime is the caller's.

**2. `ui/src/`** — the widget, beside the others.

**3. Tests, `ui/tests/`** — without a device, as `ui`'s other tests are: an image node laid out in
a row of fixed width gets the bounds the layout says; its record is kind IMAGE with the sheet and
texture index passed; a second image in the same frame is a second record after the first.

**4. `ui/ui.md`** entry.

## What must not change

- Every other widget's records and their order.
- `render`. The element record. No texture loading in `ui`.
- No image scaling modes, no aspect fitting, no tint argument. The caller sizes the node.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R ui` passes.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

`voe_ui_image` exists, is tested without a graphics card, and emits exactly one IMAGE record per
call.

## Notes
