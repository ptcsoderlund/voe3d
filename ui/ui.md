# ui

Nested rows and columns of boxes in millimetres, a rectangle for every one of
them, and the first widgets on top: a panel, a label and a button that answers
the mouse. Not drawing — what comes out is element records and the caller
submits them — and not input either: the pointer is a value it is handed. Where
the surface sits in the world is one matrix and it is the caller's.

- `include/ui/layout.h` — the context, the frame, and rows and columns and
  boxes between its begin and its end, with a rectangle read back through a
  handle. Its header says why nothing is laid out until the frame ends and why
  that is what makes the first frame right, why a call returns a handle and not
  a size, why nothing survives a frame, that the space is millimetres with Y
  down from the panel's top-left corner because that is the space an element
  record is already in, why that is not a departure from the world being Y-up,
  that a column runs from the top down so it reads in call order, that START is
  left and top on either axis, which of the three sizings may be used where,
  that a child's own fixed size across the flow beats the container's FILL, that
  overflow is reported rather than shrunk, and the three ways a frame can be
  refused.
- `include/ui/widgets.h` — the panel, the label and the button, the pointer
  they are given, and this frame's element records read back. Its header says
  why the answer to a click arrives after the frame has ended rather than at the
  call, what a widget's key is made of and why it is a hashed path and not a
  line number, what two widgets sharing one does, why a button is composed
  rather than handed a string, why a fully transparent panel emits nothing, and
  how the text scale composes with the surface's own.
- `src/context.h` — the tree and the context, shared by the folder's two
  source files. Its header says why there is one context and not two, which half
  owns which field, and why the widget pass runs where it does.
- `src/layout.c` — the tree, and the two sweeps over it. Its header says why the
  array being in call order makes both passes flat loops with neither recursion
  nor a stack, why a node's natural size is written by its parent rather than by
  itself, what a grow child contributes to a natural container and why that
  answer and not the two others, why a gap belongs to the run and not to a child,
  where the single subtraction of padding lives, why there is no flip and no
  minus sign in front of a Y anywhere in it, and why the structs are declared
  next door.
- `src/widgets.c` — what a node means, what the pointer is doing to it, and the
  records that come out. Its header says why emission is a copy with no
  arithmetic in it and where the one sign that does appear comes from, why paint
  order is taken from layout rather than re-derived, why the hit test is after
  arrange, what the two ids do in every awkward case including the stuck one,
  and why the key is FNV-1a over a path.
- `tests/layout.c` — rectangles worked out by hand, one case per decision. Its
  header says which four answers it is pinning down rather than merely
  exercising, and which two cases are about the machinery instead of the
  arithmetic. Needs no graphics card and no window system.
- `tests/widgets.c` — a press and a release in every order a hand can produce, a
  duplicate key, and a known tree emitted as a known list. Its header says why
  the click cases are the ones that matter, why the collision case is the most
  valuable in the file, and why the one case that measures a string takes a
  headless device while every other needs no graphics card. Needs no window
  system.
