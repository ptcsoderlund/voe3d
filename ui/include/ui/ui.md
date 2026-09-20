# ui

The public headers, one entry each; the fuller account of all four stays on
`ui/ui.md`.

- `colour.h` — the swatch and the colour picker, taking and handing back a
  linear colour, and what a picker costs.
- `layout.h` — nested rows and columns of boxes in millimetres and a rectangle
  for every one of them, with clipping, scroll offsets and a measured size read
  back through a handle; it lays out and does nothing else.
- `theme.h` — the five authored values of a theme, the palette of roles
  derived from them, the default inputs and the derivation, which cannot fail.
- `widgets.h` — the theme set on the context or pushed over a subtree, the
  panel, the label, the button, the choice, the number box, the text field,
  the image and the scroll area, the pointer and the keyboard they are given, and this
  frame's element records read back.
