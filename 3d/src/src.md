# src

`3d`'s implementation: one file per public header, plus the built-in shapes' geometry. Nothing here
is included from outside the folder — `include/3d/` is the whole public surface.

- `projection.c` — the projection arithmetic, written out, because the signs are
  the whole thing, and the render view a pose and a lens become.
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
- `pick.c` — the pixel's ray from the view's two matrices inverted, and the walk that
  tests it against each shape's triangles, then the cameras' marker boxes.
- `outline.c` — the walk over one shape's edges that keeps the ones the eye is on two sides of, and
  the quad each of them becomes, a half width per vertex from that vertex's own depth.
- `camera_marker.c` — the marker's twenty edges in the camera's own space, their quads with
  outline.c's width and winding mirrored, and the slab test against the box.
- `gizmo.c` — the shaft that covers the same pixels at any distance, the ray against each handle
  nearest first, the point a drag is measured from, and the two meshes of camera-facing quads it is
  all drawn as.
- `depth_sort.c` — the insertion sort, where the sign is the whole of it.
- `draw_system.c` — the camera's view and the sun, the solid pass and the blended one furthest
  first, a camera marker with the world's solids, then the outline and the move gizmo.
- `import.c` — the three uploads in their forced order and the tree walk that
  turns a read model into one entity per drawn primitive.
