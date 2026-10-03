# tests

`ui`'s own tests: plain C programs with an ordinary `main()`, zero for pass,
found by the build and registered nowhere. None needs a window system, and
only the cases that measure a string take a headless device — everything else
this folder computes is arithmetic checked against numbers worked out by hand.

- `button.c` — that a press and a release in every order a hand can produce give the right answer,
  that a half-clipped button answers only where it is seen, that a container declared to take the
  pointer clears what is under it, that a held or selected control draws `inverse`, and
  (`button_pad_follows_spacing`) that the theme's spacing scales a button's pad.
- `colour.c` — that a swatch is one record of its colour, a press at the
  square's top-right with hue nought is red, `#FFC800` and `ffc800` are taken,
  `#12` is refused leaving the colour, a press outside says so, and a colour
  made grey keeps its hue into the next frame.
- `field.c` — that a field focuses on the press, types what this frame's keys say, commits on Enter,
  Tab and a press elsewhere, cancels on Escape, wraps Tab between two fields, arrives selected and
  drawn inverted, and that a number box opens for typing on a click and never on a drag.
- `layout.c` — that the rectangles are the ones worked out by hand, one case per
  decision that could have gone the other way, wraps, clips, clamped scroll
  offsets and paint order among them.
- `number.c` — that a sideways drag on a number box moves the value, a fine one a tenth as far, a
  drag past the edge goes on moving it, the dead zone moves nothing, and a box scrolled away
  mid-drag keeps dragging.
- `scroll.c` — that a scroll area remembers its offset and clamps it, forgets it when it is not
  called, passes outward what it cannot take, and draws a bar only when it has to, whose thumb
  drags, pages and stands in front of a button.
- `slider.c` — that the thumb is against the track's left edge at the range's bottom, its right edge
  at the top and halfway between at the middle, that a drag past the dead zone moves the value by
  the distance times the range over the width, and that an untouched frame hands the value back.
- `theme.c` — that OKLab round-trips, that both modes are legible at both ends of both scalars, that
  every role shares the authored hue and a grey hue leaves no chroma, that each input moves what it
  names and nothing else, and that dark mode clamps chroma harder.
- `widgets.c` — that a duplicate key refuses the frame and the same name under two panels is two
  widgets, that a known tree is emitted as a known list in order, that a transparent panel emits
  nothing, that the nearest theme wins, and that a label is what the theme and the font say.
