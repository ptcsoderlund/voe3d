# tests

`ui`'s own tests: plain C programs with an ordinary `main()`, zero for pass,
found by the build and registered nowhere. Neither needs a window system, and
only the cases that measure a string take a headless device — everything else
this folder computes is arithmetic checked against numbers worked out by hand.

- `layout.c` — that the rectangles are the ones worked out by hand, one case per
  decision that could have gone the other way, wraps, clips, clamped scroll
  offsets and paint order among them.
- `theme.c` — that OKLab round-trips, that both modes are legible at both ends
  of both scalars, that each input moves what it names and nothing else, and
  that dark mode clamps the accent's chroma harder than light.
- `widgets.c` — that a press and a release in every order a hand can produce
  give the right answer, and so do a sideways drag, a clipped button and label,
  a scroll area's remembering, a duplicate key, a known tree emitted as a known
  list with every border and fill in order, the nearest theme winning and an
  unbalanced push refusing the frame, and a field's focus, typing, Backspace,
  Enter and capacity.
