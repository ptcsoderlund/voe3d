# tests

One plain C program per module, found by the build, pinning down the answers
each decision took rather than exercising the code. Neither needs a window
system.

- `layout.c` — rectangles worked out by hand, one case per decision, wraps,
  clips and clamped scroll offsets among them. Its header names the answers it
  is pinning down rather than merely exercising, group by group and each one a
  decision that could have gone the other way, which two cases are about the
  machinery instead of the arithmetic, and why exactly one case reaches into
  `src/` — paint order is an order and no rectangle can show it. Needs no
  graphics card and no window system.
- `widgets.c` — a press and a release in every order a hand can produce, a
  sideways drag in every order one can, a clipped button and label, a drag
  scrolled out of sight, a scroll area's remembering, passing on, bar, drag,
  page and refusal, a duplicate key, a known tree emitted as a known list, two
  images as two IMAGE records, and a field's focus, typing, Backspace, Enter and
  capacity. Its header says why the click cases are the ones that matter, why
  the collision case is the most valuable in the file, and why every case that
  measures a string — the text scale, a label's own emission, and a field, which
  always composes one — takes a headless device while the rest need no graphics
  card. Needs no window system.
