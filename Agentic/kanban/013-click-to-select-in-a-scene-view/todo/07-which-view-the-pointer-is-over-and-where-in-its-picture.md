# 07 — Which view the pointer is over and where in its picture
folder: editor
decisions: 0168, 0202

## Change
A scene view already knows its rectangle on the surface and the size of the picture it drew into it; nothing
yet turns a pointer into a place in that picture. `voe_3d_pick_ray` (card 02) wants exactly that place.

`editor/src/view.h`, beside `voe_editor_views_drag`:

```c
// Which view the pointer is over and where in that view's picture, in the
// picture's own pixels — x right, y down from its top-left corner, which is
// what voe_3d_pick_ray takes. False when the pointer is over no view, and the
// two out-parameters are untouched then. `pointer` is in the root surface's
// millimetres, the same place the drag is handed.
[[nodiscard]] bool voe_editor_views_under(const voe_editor_views *views,
                                          voe_math_float2 pointer,
                                          uint32_t *view,
                                          voe_math_float2 *point);
```

`editor/src/view.c`: walk the views in use; skip one whose `image` is `VOE_UI_NODE_NONE` — its panel was not
drawn, so its rectangle names nothing this frame, which is the same test the drag already makes about a view it
may capture. For the first whose `rect` holds the pointer, hand back its number and
`((pointer - rect.min) / rect.size) * (width, height)` — the picture is drawn at `width` by `height` pixels and
shown stretched over `rect`, so the fraction across the rectangle is the fraction across the picture.

`view.h`'s header gains a paragraph: WHERE A CLICK LANDS IS LAST FRAME'S RECTANGLE, FOR THE SAME REASON THE
PICTURE'S SIZE IS. The rectangle comes out of `ui` after the frame it was laid out in, so a click is read
against the rectangle the dock walk recorded last frame — exactly what the middle-button drag is already
measured against — and the aspect ratio the ray is built with is the picture's own `width` and `height`, which
is what it was actually drawn with, not the rectangle's.

`editor/src/src.md`'s `view.h` line says it also answers which view a pointer is over and where in its picture.

## Done when
`checks.sh editor` exits 0 and `cmake --build --preset debug` builds the tree; the call is used by card 08.
