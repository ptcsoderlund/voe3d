# 02 — Static geometry and shading records can be freed
folder: render
decisions: 0168, 0278

## Change
Additive to the public surface (0278 point 2).

- `render/include/render/device.h`:
  - `bool voe_render_geometry_destroy(voe_render_device *device, voe_render_geometry geometry);`
    beside `voe_render_geometry_create`: gives the range back to both static pools and the slot
    back with its generation bumped; false for a stale id (not an error, as texture destroy);
    a transient id asserts; waits for idle, a startup operation. Rewrite the create's
    "NEVER FREED" paragraph to say ranges now come back through this.
  - `bool voe_render_shading_destroy(voe_render_device *device, voe_render_shading shading);`
    beside `voe_render_shading_create`, the same contract for a record's slot.
- `render/src/device_parts.h`: the pool gains a free-range list (offset, count) with room for
  as many ranges as the band has geometry slots; the static geometry slot and the shading slot
  gain whatever says free.
- `render/src/geometry.c`: create takes the first free range that fits (splitting it) before
  appending at `used`; destroy returns the vertex and the index range to their pool's list,
  sorted by offset, merged with a neighbour it touches, and a range touching `used` lowers
  `used` instead. A slot freed is reused by the next create with its generation bumped. Rewrite
  the file header's "NOTHING STATIC IS FREED" paragraph.
- `render/src/shading.c`: destroy frees the slot; create takes a free slot first. Header updated.
- `render/tests/pools.c`: cases — a device with room for exactly two meshes creates two,
  destroys the first, creates a third of the same size, and all three ids behave (the first
  refused by a draw, the others drawn); destroying twice returns false; two neighbouring ranges
  freed then one range of their summed size fits; a shading record destroyed then created
  reuses the slot and the old id is refused. Update its line in `render/tests/tests.md`.
- `render/src/src.md`: the `geometry.c` and `shading.c` lines say ranges and records are freed.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_render $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_render_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^render/"` exits 0.
