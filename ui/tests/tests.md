# tests

`ui`'s own tests: plain C programs with an ordinary `main()`, zero for pass,
found by the build and registered nowhere. None needs a window system, and
only the cases that measure a string take a headless device — everything else
this folder computes is arithmetic checked against numbers worked out by hand.

- `button.c` — that a press and a release in every order a hand can produce give
  the right answer, that a half-clipped button is drawn and answers the pointer
  only where it is seen, and that a held button, a number box being dragged and a
  selected choice draw `inverse` with the label on them in `inverse_ink` while a
  hovered one stays `control_hovered`; the module under it is `ui/src/button.c`.
- `colour.c` — that a swatch is one record of its colour, a press at the
  square's top-right with hue nought is red, `#FFC800` and `ffc800` are taken,
  `#12` is refused leaving the colour, a press outside says so, and a colour
  made grey keeps its hue into the next frame.
- `field.c` — that a field focuses on the press itself, types what this frame's
  keys say, takes a code point whole backward and at capacity, arrives with its
  text selected, commits on Enter, on Tab and on a press elsewhere, cancels on
  Escape, wraps Tab between two fields, ignores control bytes and carries
  `voe_ui_typing` with the focus, that the text it arrives with is drawn
  inverted — a record of `inverse` behind letters in `inverse_ink` — and that a
  number box opens for typing on a click and never on a drag, takes a typed
  number on Enter and stays open on one it refuses.
- `layout.c` — that the rectangles are the ones worked out by hand, one case per
  decision that could have gone the other way, wraps, clips, clamped scroll
  offsets and paint order among them.
- `number.c` — that a sideways drag on a number box moves the value, a fine one
  moves it a tenth as far, a drag past the edge goes on moving it, half a
  millimetre inside the dead zone moves nothing and a box scrolled away mid-drag
  keeps dragging; the module under it is `ui/src/button.c` too, the one module
  two programs cover.
- `scroll.c` — that a scroll area remembers its offset and clamps it, forgets it
  when it is not called, passes outward what it cannot take, and draws a bar only
  when it has to, whose thumb is as long and as far as the offset says, is
  `inverse` while it is held and the control at rest, drags, pages and stands
  in front of a button — and that one area too many refuses the frame.
- `slider.c` — that the thumb is against the track's left edge at the range's
  bottom, against its right edge at the top and halfway between at the middle,
  that a sideways drag past the dead zone moves the value by the distance times
  the range over the width, that a drag past either end answers exactly that
  end, and that a frame nobody touched hands the value straight back.
- `theme.c` — that OKLab round-trips, that both modes are legible at both ends
  of both scalars and the inverted pair with them, that every role shares the
  authored hue and a grey hue leaves no chroma anywhere, that each input moves
  what it names and nothing else, and that dark mode clamps the hue's chroma
  harder than light.
- `widgets.c` — that a duplicate key refuses the frame and the same name under
  two panels is two widgets, that a known tree is emitted as a known list with
  every border and fill in order and two images in call order among them, that a
  transparent panel emits nothing, that the nearest theme wins and an unbalanced
  push refuses the frame, and that a label's letters, its size and what a clip
  leaves of it are what the theme and the font say.
