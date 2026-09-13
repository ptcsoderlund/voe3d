# ui

Nested rows and columns of boxes in millimetres, a rectangle for every one of
them, and the first widgets on top: a panel, a label, a button that answers the
mouse, a number box you drag sideways to change a value and an image. Not drawing — what
comes out is element records and the caller submits them — and not input either:
the pointer is a value it is handed. Where the surface sits in the world is one
matrix and it is the caller's.

- `include/ui/layout.h` — the context, the frame, and rows and columns and
  boxes between its begin and its end, with a rectangle and a measured size read
  back through a handle. Its header says why nothing is laid out until the frame
  ends and why that is what makes the first frame right, why a call returns a
  handle and not a size, why nothing survives a frame, that the space is
  millimetres with Y down from the panel's top-left corner because that is the
  space an element record is already in, why that is not a departure from the
  world being Y-up, that a column runs from the top down so it reads in call
  order, that START is left and top on either axis, which of the three sizings
  may be used where, that a child's own fixed size across the flow beats the
  container's FILL, that overflow is reported rather than shrunk, and the three
  ways a frame can be refused. It says that X is laid out for the whole tree
  before Y and that nothing may need a height to know a width, and how a
  container that asks to wrap breaks its run into lines — and why a wrapping
  column overflows to the right instead of widening. It also says why padding is four numbers named by
  absolute side, why there is no margin and what to do instead, what an anchored
  child is and why its two axes are X and Y rather than the flow's two words,
  which way its offset moves it, that it is measured against its parent's
  content box and paints over its in-flow siblings, the trap that a
  fit-to-children parent holding only anchored children has no natural size at
  all, and what the measured size is for.
- `include/ui/widgets.h` — the panel, the label, the button, the number box and
  the image, the pointer they are given, and this frame's element records read
  back. Its
  header says why the answer to a click arrives after the frame has ended rather
  than at the call, what a widget's key is made of and why it is a hashed path
  and not a line number, what two widgets sharing one does, why a button is
  composed rather than handed a string, why a fully transparent panel emits
  nothing, and how the text scale composes with the surface's own. On the number
  box it says why this folder knows no field kinds and takes a value and a rate
  instead, why what comes back is a value and not a distance and what that buys
  the typing that is not built yet, and why a press and release without movement
  is reserved rather than free. On the image it says what it is for, why it is
  sized as a box is, that it is one element, and that the texture's lifetime is
  the caller's.
- `src/context.h` — the tree and the context, shared by the folder's two
  source files. Its header says why there is one context and not two, which half
  owns which field, why the widget pass runs where it does, and why a drag needs
  four fields beside `held` and no keyed table.
- `src/layout.c` — the tree, and the sweeps over it, one axis at a time. Its
  header says why the array being in call order makes every pass a flat loop
  with neither recursion nor a stack, what passes between the X pass and the Y
  pass, where a wrapping column has to revisit X, why a node's natural size is
  written by its parent
  rather than by itself, what a grow child contributes to a natural container and
  why that answer and not the two others, why a gap belongs to the run and not to
  a child, how an anchored child is a stronger exclusion than a grow one and why
  its axes are absolute, how paint order is worked out in three linear sweeps now
  that it is no longer the array's own order, where the single subtraction of
  padding lives and why four numbers still go through two accessors, why there is
  no flip and no minus sign in front of a Y anywhere in it, and why the structs
  are declared next door.
- `src/widgets.c` — what a node means, what the pointer is doing to it, and the
  records that come out. Its header says why emission is a copy with no
  arithmetic in it and where the one sign that does appear comes from, why paint
  order is taken from layout rather than re-derived, why the hit test is after
  arrange, what the two ids do in every awkward case including the stuck one,
  why the key is FNV-1a over a path, and why a drag measures its dead zone from
  the press and its change from last frame.
- `tests/layout.c` — rectangles worked out by hand, one case per decision. Its
  header says which eight answers it is pinning down rather than merely
  exercising, which two cases are about the machinery instead of the arithmetic,
  and why exactly one case reaches into `src/` — paint order is an order and no
  rectangle can show it. Needs no graphics card and no window system.
- `tests/widgets.c` — a press and a release in every order a hand can produce, a
  sideways drag in every order one can, a duplicate key, a known tree emitted as
  a known list, and two images as two IMAGE records. Its header says why the click cases are the ones that matter,
  why the collision case is the most valuable in the file, and why the one case
  that measures a string takes a headless device while every other needs no
  graphics card. Needs no window system.
