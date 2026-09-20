# src

`ui`'s implementation: the tree and the sweeps that settle it, then what a node
means once the pointer and the keyboard have been at it. Nothing here is
included from outside the folder — `include/ui/` is the whole public surface.

The seam runs by pass. `layout.c` knows rectangles and nothing about identity or
input; `widgets.c` knows keys, themes, the frame and records and asks `layout.c`
for every number it needs; `button.c`, `field.c` and `scroll.c` are the widgets
with a gesture of their own, built out of what `widgets.c` offers; `colour.c`
builds its widgets out of those and answers them at the frame's end;
`slider.c` is composed from the public calls alone; and
`context.h` is the one struct they all share, with the entry points each of them
offers the others written at its end.

- `button.c` — the button, the choice and the number box: how each is built,
  what the pointer comes to on any widget — the hit test, what a press arms,
  what a release fires and the drag between them — and the fill and the border
  by state they draw, inverted while one is held, dragged or selected.
- `colour.c` — the swatch and the colour picker: HSV of sRGB, the hue each
  picker remembers under its key, a press or drag in the square or strip and a
  hex commit turned into a colour at the frame's end, and the cells and markers.
- `context.h` — the node and the one context every file here shares, each
  field owned by one of them: the capacities, the tree, the pointer and the
  keyboard, the drag in progress and what is held and focused, the theme stack
  each node copies its theme from once, at the call that made it, and
  `colour.c`'s pickers, with the entry points each file offers the others.
- `field.c` — the single-line text field and the one keyboard focus: where a
  press takes it, Tab order, what this frame's keys type, commit and cancel,
  typing into an open number box, and the caret, the selection and the text they
  draw.
- `layout.c` — the tree, and the sweeps over it, one axis at a time: natural
  sizes, the X pass, the wraps, the Y pass, the corrective sweep, the clamped
  scroll offsets, the clips and the paint order.
- `oklab.h` — the OKLab conversion the theme is derived in, kept here rather
  than in `math`, with its own L, a and b struct and a clamp at the gamut's edge,
  and the sRGB transfer function both ways it is built on.
- `oklab.c` — the conversion both ways between linear sRGB and OKLab, the
  clamp back into gamut, and the sRGB transfer function the picker shares.
- `scroll.c` — the scroll area: the table of offsets that outlives a frame, the
  bars over its content against the pointer and into the records, and every way
  an offset moves — a thumb dragged, a track pressed, the pointer's scroll
  passed outward.
- `slider.c` — the slider: a number box holding a fixed-width track with the
  thumb anchored at the value's place in the range, and the clamp that keeps a
  drag from taking the value out of it.
- `theme.c` — the derivation: every role in the one authored hue, differing
  only in lightness and shrinking chroma where the gamut makes it, the ground,
  surface, raised and control ladder in equal steps of `surface_separation`,
  text and the border stepped from `ground` until they clear their surface, and
  the inverse pair a held, dragged or selected control is drawn in.
- `widgets.c` — what a node means and the records that come out, and everything
  every widget shares: the hashed keys, the theme in force, the pointer and the
  keyboard as they are handed in, the frame's two boundaries, the walk that
  emits records in paint order and dispatches per widget, and the widgets that
  answer nothing — the panel, the label and the image.
