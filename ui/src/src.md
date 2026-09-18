# src

`ui`'s implementation: the tree and the sweeps that settle it, then what a node
means once the pointer and the keyboard have been at it. Nothing here is
included from outside the folder — `include/ui/` is the whole public surface.

The seam runs by pass. `layout.c` knows rectangles and nothing about identity or
input; `widgets.c` knows keys, hit tests and records and asks `layout.c` for
every number it needs; and `context.h` is the one struct they share.

- `context.h` — the node and the one context the two source files share, each
  field owned by one half or the other: the capacities, the tree, the pointer
  and the keyboard, the drag in progress and what is held and focused.
- `layout.c` — the tree, and the sweeps over it, one axis at a time: natural
  sizes, the X pass, the wraps, the Y pass, the corrective sweep, the clamped
  scroll offsets, the clips and the paint order.
- `widgets.c` — what a node means, what the pointer and the keyboard are doing
  to it, and the records that come out: the hashed keys, the hit test against
  the visible rectangle, the drags, the scroll table, the field's editing and
  the clipped element records.
