# 066 — `ui` has an image

claimed-by: claude-opus-5 (session 01TYrewyPCZyspC4KzToTd3K)
blocked-by: 065
status: review
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

**Implemented, and verified on Linux only** (Fedora 44, clang 22). Windows not checked.

- **Signature follows `voe_ui_box`, not the sketch, as Scope 1 allows.**
  `voe_ui_image(ui, texture, sheet, content, sizing)`. `ui` sizes a leaf with
  `voe_math_float2 content` plus `voe_ui_sizing` (along/across), not a width and a
  height, so the image takes the box's two arguments. It is a box with a picture.
- **No name.** A label takes none because it has no identity, and an image has
  nothing to remember either, so it claims no key.
- **Pointer query: `ui` has none for a node that is not a button or number box.**
  A panel can't be asked either. The header tells a caller to compare the pointer
  with `voe_ui_node_rect`. Nothing added.
- Internal: `VOE_UI_WIDGET_IMAGE` plus `sheet` and `texture` fields on the widget
  record (`src/context.h`), and `push_image` in the emission switch (`src/widgets.c`).
  The header, the test file header and `ui.md` are updated.
- Test `two_images_are_two_records_in_call_order`: a fixed 100 mm row holding a
  fixed image and a grow image, with rectangles worked out by hand. Each image is
  one IMAGE record, clipped to its own bounds and opaque white, with the sheet and
  texture index it was given. The two test ids differ in both halves, so carrying
  the generation instead of the index fails.
- Ran: `cmake -P check.cmake`, all steps ok (45 tests, analyser 122 files).
  `ctest -R ui`: 2/2 passed. Probe: `sheet_texture` changed to `index + 1` made
  both record checks fail. The probe was reverted and the test passes again.
  `tools/hot.sh`: card 58/150, `ui.md` 74/120, no OVER.
- No DEVIATION or BLOCKED markers.
