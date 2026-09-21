# ui

Nested rows and columns of boxes in millimetres, a rectangle for every one of
them, and the widgets on top: a panel, a label, a button, a number box, a text
field, an image, a scroll area, a swatch, a colour picker and a slider. It
draws nothing and reads no device: it hands back element records, and the
pointer and the keyboard are values it is given.

- `include` — the public headers, in `include/ui/`; each is listed below by path.
- `src` — the implementation: the tree and the sweeps that settle it, then what
  a node means once the pointer and the keyboard have been at it; each file is
  listed on `src/src.md`.
- `tests` — one plain C program per module, found by the build, none of them
  needing a window system; each is listed on `tests/tests.md`.
- `include/ui/colour.h` — the swatch, one solid element of a linear colour,
  and the colour picker (`voe_ui_colour_picker`, `voe_ui_colour_picker_action`,
  `voe_ui_colour_result`): a saturation/value square, a hue strip and a hex
  field in a panel the caller places.
- `include/ui/layout.h` — the context, the frame, and rows, columns and boxes
  in millimetres with Y down: their sizing, padding, wrapping, anchoring,
  clipping and scroll offsets, and each node's rectangle, visible part and
  measured size read back after the frame ends; the file says why.
- `include/ui/slider.h` — the slider (`voe_ui_slider`, `voe_ui_slider_action`,
  `voe_ui_slider_result`, `VOE_UI_SLIDER_HEIGHT`, `VOE_UI_SLIDER_THUMB`): a
  track of a given width with a thumb at the value's place in a range.
- `include/ui/theme.h` — `voe_ui_theme_inputs` (the five authored values),
  `voe_ui_theme` (the derived palette of roles), `voe_ui_theme_default_inputs`
  and `voe_ui_theme_derive`, which turns the one into the other in OKLab.
- `include/ui/widgets.h` — the theme set on the context or pushed over a
  subtree, the panel, the label, the button, the choice, the number box, the
  text field, the image and the scroll area, the pointer and the keyboard they
  are given, and this frame's element records read back.
