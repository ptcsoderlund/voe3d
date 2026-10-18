# 25 — A geometry can walk another geometry's vertices
folder: render
after: 24
decisions: 0168, 0444

## Change
0444: a model's made levels reuse level 0's vertices and add only their own indices. `render`
gains the create that makes that possible; `3d` calls it in card 32.

- `render/include/render/device.h` — after `voe_render_geometry_create`:
  `[[nodiscard]] bool voe_render_geometry_create_sharing(voe_render_device *device,
  voe_render_geometry vertices_of, const uint32_t *indices, uint32_t index_count,
  voe_render_geometry *out, voe_base_error *error)`.
  Comment points: a static geometry whose indices, numbered from zero as ever, walk the
  vertices of `vertices_of`, a live static geometry; only the indices are uploaded; it draws,
  casts and is boxed like any other, its sphere and box being its base's; a startup operation that
  waits for the card like create; fails as create fails (no slot, a refused grow of the index pool,
  a refused upload); a stale or transient base, a count of zero, or an index at or past the base's
  vertex count is the caller's bug and asserts. Destroy's comment: a sharing geometry gives back its
  indices alone; destroying a base while a sharing geometry still walks it asserts, so a caller
  destroys the sharers first.
- `render/src/device_parts.h` — `struct voe_render_geometry_slot` gains the slot index of the base
  it shares (none for its own vertices) and, on a base, how many share it; comments say who writes
  each.
- `render/src/geometry.c` — the new create: checks, a slot, an index range (growing the index pool
  as card 24's create does), the upload of the indices alone, the vertex offset and count, sphere
  and box copied from the base, the base's count raised. `destroy_geometry`: a sharing slot gives
  back its index range only and lowers its base's count; a base with a count above nought asserts.
  Header: a paragraph on sharing slots, why (0444), and the destroy order.
- `render/src/src.md` — the `geometry.c` entry names the sharing create.
- `render/tests/pools.c` — new `a_sharing_mesh_walks_its_base`: a four-vertex, six-index base and
  a three-index sharer: the sharer's vertex offset and count are the base's, its index range is its
  own and overlaps none, its box is the base's, `draw_each` draws it; destroying the sharer then the
  base frees both, each id refused after, and the base's vertex range is reused by the next create.
- `render/tests/tests.md` — the `pools.c` entry names the sharing mesh.

## Done when
`ctest --test-dir build/debug -R '^render/pools$'` passes with `a_sharing_mesh_walks_its_base`.
