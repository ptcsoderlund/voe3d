# src

`3d`'s implementation: one file per header above it, plus the built-in cube's
data, which only the shape system uses.

- `projection.c` — the arithmetic, written out, because the signs are the whole
  thing.
- `normal_matrix.c` — the inverse transpose, the derivation in three lines, and
  the one branch a flattened object needs.
- `mesh_component.c` — the key, the registration as runtime-only, the creation
  call, the geometry change and the reads.
- `panel_component.c` — the same, with the range in place of the geometry.
- `material_component.c` — the same, runtime-only too, plus the upload that turns
  a material's numbers into a record. Its header says why the UV rect is the one
  field that is not a straight copy.
- `shape_component.c` — the key, the registration as described, the creation call
  and the reads.
- `shape_system.c` — the upload and the run. Its header says why the run and its
  unknown-kind count are per process.
- `cube.h` — the built-in cube's vertices and indices, by hand. Internal: only
  shape_system.c uses it. Its header says why twenty-four vertices and not eight
  and why every face winds counter-clockwise.
- `cube.c` — those vertices and indices themselves. Its header says which corner
  of a face the first vertex is and which way its texture coordinates run.
- `depth_sort.c` — the insertion sort, and the sign is the whole of it.
- `draw_system.c` — the camera, the sun, two matrices per object, the solid pass
  in table order and the blended one furthest first, on each side of the
  overlay's depth clear, over both tables. Its header says why there are four
  groups, why only one of them can be drawn as a table is walked, why the order
  of the two walks decides nothing, why the overlay has a solid group from the
  start, and why an empty overlay clears nothing. `struct deferred`'s comment
  says why a held-back entry is one of two things, and `range_is_this_frame_s`
  says what a stale range does and what that does not catch.
- `import.c` — the uploads and the tree walk. Its header says why the order is
  forced, why uploading the pictures has to read the materials first, why the
  walk needs no recursion, and why nothing unwinds on failure.
