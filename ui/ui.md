# ui

Nested rows and columns of boxes in millimetres, and a rectangle for every one
of them. Not drawing, not input, and not widgets — where the surface sits in the
world is one matrix and it is the caller's, and a button is a later card.

- `include/ui/layout.h` — the whole public surface: a context, a frame, rows and
  columns and boxes between its begin and its end, and a rectangle read back
  through a handle. Its header says why nothing is laid out until the frame ends
  and why that is what makes the first frame right, why a call returns a handle
  and not a size, why nothing survives a frame, that the space is millimetres
  with Y down from the panel's top-left corner because that is the space an
  element record is already in, why that is not a departure from the world being
  Y-up, that a column runs from the top down so it reads in call order, that
  START is left and top on either axis, which of the three sizings may be used where, that a child's own
  fixed size across the flow beats the container's FILL, and that overflow is
  reported rather than shrunk.
- `src/layout.c` — the tree, and the two sweeps over it. Its header says why the
  array being in call order makes both passes flat loops with neither recursion
  nor a stack, why a node's natural size is written by its parent rather than by
  itself, what a grow child contributes to a natural container and why that
  answer and not the two others, why a gap belongs to the run and not to a child,
  and where the single subtraction of padding lives, and why there is no flip and
  no minus sign in front of a Y anywhere in it.
- `tests/layout.c` — rectangles worked out by hand, one case per decision. Its
  header says which four answers it is pinning down rather than merely
  exercising, and which two cases are about the machinery instead of the
  arithmetic. Needs no graphics card and no window system.
