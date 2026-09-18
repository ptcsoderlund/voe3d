# ui

The public headers, one entry each; the fuller account of both of these stays on
`ui/ui.md`.

- `layout.h` — nested rows and columns of boxes in millimetres and a rectangle
  for every one of them, with clipping, scroll offsets and a measured size read
  back through a handle; it lays out and does nothing else.
- `widgets.h` — the panel, the label, the button, the number box, the text
  field, the image and the scroll area, the pointer and the keyboard they are
  given, and this frame's element records read back.
