# src

`3d`'s implementation: one file per public header, plus the built-in shapes' geometry. Nothing here
is included from outside the folder — `include/3d/` is the whole public surface.

- `projection.c` — the projection arithmetic, written out, because the signs are
  the whole thing.
- `normal_matrix.c` — the inverse transpose, its derivation in three lines, and
  the one branch a flattened object needs.
- `mesh_component.c` — the mesh table: its key, its registration as runtime-only,
  its creation call, the geometry change and the reads.
- `panel_component.c` — the same for panels, with this frame's element range in
  place of the geometry.
- `material_component.c` — the same again, runtime-only too, plus the upload that
  turns a material's numbers into a record `render` holds.
- `shape_component.c` — the shape table: its key, its registration as described
  with its default row, its need of a transform and its intent, its creation
  call and the reads.
- `shape_system.c` — the one upload of the three shapes' geometry and of the two materials they
  wear, the intent's submit and drain, and the run that gives a shape its mesh and material,
  repoints a changed kind and drops both once the shape is gone.
- `shape_geometry.c` — the three shapes kept on the CPU: the cube pointed at, the
  capsule and the cylinder built into an arena, and each surface's edges found by
  welding its vertices by position and walking its triangles twice.
- `cube.h` — the built-in cube's vertices and indices as the files that use them
  see them, internal to this folder.
- `cube.c` — those twenty-four vertices and thirty-six indices written out by
  hand, every face wound counter-clockwise seen from outside.
- `capsule.h` — the built-in capsule's counts and its builder, internal to this
  folder, and why a pole row is thirty-three copies of one point.
- `capsule.c` — the capsule's eighteen rows and the bands between them, a pole's
  band one triangle a segment.
- `cylinder.h` — the built-in cylinder's counts and its builder, internal to
  this folder, and why its rim is built three times.
- `cylinder.c` — the cylinder's side as two rows and each cap as a fan.
- `pick.c` — the pixel's ray out of the two matrices inverted once, and the walk
  over the shape table that carries it into each shape's own space and tests the
  kind's triangles, with the triangle test written out once and its derivation in
  a comment.
- `outline.c` — the walk over one shape's edges that keeps the ones the eye is on two sides of, and
  the quad each of them becomes, a half width per vertex from that vertex's own depth.
- `gizmo.c` — the shaft that covers the same pixels at any distance, the ray against each arrow's
  segment and each square's patch of plane nearest first, and the closest point on an axis or in a
  plane a drag is measured from.
- `depth_sort.c` — the insertion sort, where the sign is the whole of it.
- `draw_system.c` — the camera, the sun, two matrices and a colour per object,
  the solid pass in table order and the blended one furthest first, on each
  side of the overlay's depth clear, over both tables.
- `import.c` — the three uploads in their forced order and the tree walk that
  turns a read model into one entity per drawn primitive.
