# ui

Nested rows and columns of boxes in millimetres, a rectangle for every one of
them, and the first widgets on top: a panel, a label, a button that answers the
mouse, a number box you drag sideways or click and type into to change a
value, a single-line text field, an image, a scroll area that remembers its
offset, a swatch, a colour picker and a slider. Not drawing — what comes
out is element records and the caller submits them — and not input either:
the pointer and the keyboard are values it is handed. Where the surface sits
in the world is one matrix and it is the caller's. It also turns an
authored theme — one colour, two scalars, a mode and a text size — into the
palette a widget draws with (`include/ui/theme.h`); reading a theme file into
those authored values is a different folder's job (`theme`, ADR-0170). Every
widget draws from the nearest theme in force — set on the context or pushed
over a subtree (`voe_ui_theme_set`, `voe_ui_theme_push`/`voe_ui_theme_pop`).

- `include` — the public headers, in `include/ui/`; each is listed below by path.
- `src` — the implementation: the tree and the sweeps that settle it, then what
  a node means once the pointer and the keyboard have been at it; each file is
  listed on `src/src.md`.
- `tests` — one plain C program per module, found by the build, none of them
  needing a window system; each is listed on `tests/tests.md`.
- `include/ui/colour.h` — the swatch, one solid element of a linear colour,
  and the colour picker (`voe_ui_colour_picker`, `voe_ui_colour_picker_action`,
  `voe_ui_colour_result`): a saturation/value square, a hue strip and a hex
  field in a panel the caller places. Its header says why it is placed by the
  caller and not a popup, why the gradients are cells, how the hue survives a
  grey, and what it costs in nodes and element records.
- `include/ui/layout.h` — the context, the frame, and rows, columns and boxes
  in millimetres with Y down: their sizing, padding, wrapping, anchoring,
  clipping and scroll offsets, and each node's rectangle, visible part and
  measured size read back after the frame ends; the file says why.
- `include/ui/slider.h` — the slider (`voe_ui_slider`, `voe_ui_slider_action`,
  `voe_ui_slider_result`, `VOE_UI_SLIDER_HEIGHT`, `VOE_UI_SLIDER_THUMB`): a
  track of a given width with a thumb at the value's place in a range. Its
  header says that it IS a number box and has no widget kind of its own, so a
  drag across it changes the value, a click opens it for typing and it draws
  inverted while it is dragged, that the range is applied on the way out and
  never on the way in and why the clamp counts as a change, and what it costs
  in nodes and element records.
- `include/ui/theme.h` — `voe_ui_theme_inputs` (the five authored values),
  `voe_ui_theme` (the derived palette of roles), `voe_ui_theme_default_inputs`
  and `voe_ui_theme_derive`. Its header says why the one authored `hue` stays
  sRGB in the inputs and is linear in every derived role, why every role
  carries that hue and roles differ only in lightness, why a held, dragged or
  selected control is drawn in `inverse` with its text in `inverse_ink`, why
  the derivation runs in OKLab and cannot fail, why a NULL font is allowed,
  why the hue's chroma is clamped harder in dark mode than in light, and why
  `VOE_UI_THEME_SCALAR_MIN`/`MAX` are public — `theme` has to refuse the same
  range this folder clamps to.
- `include/ui/widgets.h` — the theme set on the context or pushed over a
  subtree, the panel, the label, the button, the choice, the number box, the
  text field, the image and the scroll area, the pointer and the keyboard they
  are given, and this frame's element records read back.
