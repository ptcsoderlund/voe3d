# 24 — The geometry pools grow
folder: render
after: 23
decisions: 0168, 0444, 0445

## Change
0444: the static vertex and index pools are a starting size. A static create that finds no free
range and no room past `used` grows the pool, then takes its range as ever. Only geometry grows.

- New `render/src/geometry_grow.c` — the growth of one static pool: a new device-local buffer
  (same usage and debug name as `build_pool` in `geometry.c` gives it) of the larger of twice the
  capacity and what is needed; if the card refuses that, once more at exactly what is needed; the
  old buffer's `[0, used)` elements copied across in one submission that is waited on; the old
  buffer and its memory destroyed; the pool's buffer, memory and capacity replaced. Ranges, the
  free list and every slot's offsets stay as they are. A need past `UINT32_MAX` elements is the
  limit of a 32-bit count, reported as that limit. Header comment: why growing is safe here
  (create already waits for the card to go idle, under the device's guard, and draws bind the
  pool's buffer when they record), the short peak of old plus new, the doubling.
- `render/src/buffer.c` — if it has no call that records and waits on a one-time buffer-to-buffer
  copy, add one beside the staging upload, reusing its submission; header line for it.
- `render/src/device_calls.h` — declare the grow (and the copy, if added) under their files.
- `render/src/geometry.c` — `create_geometry`: when `pool_find` finds no hole and the pool is spent,
  grow the pool that lacks room (vertices, indices or both) before appending. The "no room for a
  mesh" failure goes; a refused grow is `VOE_BASE_ERROR_REFUSED` with one line in 0445 point 5's
  terms: the graphics card has run out of memory, the vertices or indices asked for, the pool's
  room now, and the bytes. Header: the first-fit paragraph's "returned failure" becomes a growth;
  the opening paragraph says the static pools grow.
- `render/src/device_parts.h` — `struct voe_render_pool`'s comment: a static pool's capacity grows,
  a transient one's never.
- `render/include/render/device.h` — comments only: the capacities paragraph says `vertices` and
  `indices` are where the static pools start and grow from (0444), everything else stays fixed;
  `voe_render_geometry_create`'s FAILS paragraph: full pools grow, it fails when the card will not
  hold the larger pool, no slot is left or the upload is refused; the "two shared pools" paragraph.
- `render/src/src.md` — a `geometry_grow.c` entry; the `geometry.c` entry says the pools grow.
- `render/tests/pools.c` — `a_full_pool_says_so` becomes `a_full_pool_grows`: on a device whose
  room is one triangle, a second and a third are taken, the ranges do not overlap, the pools'
  capacities (internal header) hold all three, the first mesh's box is unchanged, `draw_each`
  draws all three; a full slot table is still refused. The header's FULL POOL paragraph says why
  a full pool now grows.
- `render/tests/tests.md` — the `pools.c` entry: a full pool grows.

## Done when
`ctest --test-dir build/debug -R '^render/pools$'` passes with `a_full_pool_grows`.
