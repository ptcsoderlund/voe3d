# src

`ui`'s implementation: the tree and its sweeps, and what a node means once it
has a rectangle. The two source files share one context, declared next door.

- `context.h` — the tree and the context, shared by the folder's two source
  files. Its header says why there is one context and not two, which half owns
  which field, why the widget pass runs where it does, why a drag needs four
  fields beside `held` and no keyed table, why a held thumb needs no key of its
  own, and what `focus` is beside `held`.
- `layout.c` — the tree, and the sweeps over it, one axis at a time. Its header
  says why the array being in call order makes every pass a flat loop with
  neither recursion nor a stack, what passes between the X pass and the Y pass,
  where a wrapping column has to revisit X, why a corrective sweep after both
  passes measures each axis again and re-clamps every offset once the wraps are
  decided, why a node's natural size is written by its parent rather than by
  itself, what a grow child contributes to a natural container and why that
  answer and not the two others, why a gap belongs to the run and not to a
  child, how an anchored child is a stronger exclusion than a grow one and why
  its axes are absolute, how paint order is worked out in three linear sweeps
  now that it is no longer the array's own order, where a scroll offset is
  clamped and how the clip is a fourth flat sweep, where the single subtraction
  of padding lives and why four numbers still go through two accessors, why
  there is no flip and no minus sign in front of a Y anywhere in it, and why the
  structs are declared next door.
- `widgets.c` — what a node means, what the pointer and the keyboard are doing
  to it, and the records that come out. Its header says why emission is a copy
  with no arithmetic in it and where the one sign that does appear comes from,
  why paint order is taken from layout rather than re-derived, why the hit test
  is after arrange and against the visible rectangle, how a record is clipped,
  what the two ids do in every awkward case including the stuck one, why the key
  is FNV-1a over a path, why a drag measures its dead zone from the press and
  its change from last frame, how the scroll table is rewritten each frame and
  where a scrollbar sits in paint order, why a field's focus follows the press
  itself rather than `held`/`fired`'s release, and why its editing runs after
  resolve rather than inside it.
