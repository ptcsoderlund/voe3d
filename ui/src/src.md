# src

`ui`'s implementation: the tree and the sweeps that settle it, then what a node
means once the pointer and the keyboard have been at it. Nothing here is
included from outside the folder — `include/ui/` is the whole public surface.

The seam runs by pass. `layout.c` knows rectangles and nothing about identity or
input; `widgets.c` knows keys, hit tests and records and asks `layout.c` for
every number it needs; and `context.h` is the one struct they share.

- `context.h` — the node and the one context the two source files share, each
  field owned by one half or the other: the capacities, the tree, the pointer
  and the keyboard, the drag in progress and what is held and focused, and the
  theme stack each node copies its theme from once, at the call that made it.
- `layout.c` — the tree, and the sweeps over it, one axis at a time: natural
  sizes, the X pass, the wraps, the Y pass, the corrective sweep, the clamped
  scroll offsets, the clips and the paint order.
- `oklab.h` — the OKLab conversion the theme is derived in, kept here rather
  than in `math`, with its own L, a and b struct and a clamp at the gamut's edge.
- `oklab.c` — the conversion both ways between linear sRGB and OKLab, and the
  clamp back into gamut.
- `theme.c` — the derivation: every role grey but the accent, the ground,
  surface, raised and control ladder in equal steps of `surface_separation`,
  and text and the border stepped from `ground` until they clear their surface.
- `widgets.c` — what a node means, what the pointer and the keyboard are doing
  to it, and the records that come out: the hashed keys, the hit test against
  the visible rectangle, the drags, the scroll table, the field's editing, the
  theme role every widget's colours come from and the clipped element records,
  a panel's and a button's hairline border among them.
