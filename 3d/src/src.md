# src

`3d`'s implementation: one file per public header, plus the built-in shapes' geometry. Nothing
here is included from outside the folder — `include/3d/` is the whole public surface.

- `projection.c` — the projection arithmetic, written out, because the signs are
  the whole thing, and the render view a pose and a lens become.
- `shadow_cascades.c` — the splits, each slice's sphere from the inverted
  projection, and each cascade's snapped light view and box.
- `light_box.h` — the sun's basis, the snap to whole texels in double and the
  look from the sun, shared by the cascades and the bounce grid; internal.
- `light_box.c` — those three, moved out of shadow_cascades.c unchanged.
- `bounce_grid.c` — forward out of the view, the grid's whole cells in double,
  its corner about the eye, and the sun's view of its sphere snapped to blocks.
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
  call, the reads and the collider that fits each kind.
- `model_component.c` — the model table: its key, its registration, the reads, the
  intent's submit and the drain that cuts a path with no end.
- `emitter_component.c` — the emitter and particles tables: their keys, the registration with
  0298's defaults, the reads and the two submits.
- `emitter_system.c` — the two drains, the particles rows added and dropped, the births in a
  cone through the world matrix from a xorshift, and the step that moves and kills.
- `water_component.c` — the water and waves tables: their keys, the registration with 0305's
  defaults, the reads and the replace submit.
- `water_system.c` — the replace drain, the waves rows added and dropped, and the clock stepped
  in double and wrapped with fmod.
- `shape_system.c` — the one upload of the three shapes' geometry and of the two materials they
  wear, the intent's submit and drain, and the run that gives a shape its mesh and material,
  repoints a changed kind and drops both once the shape is gone.
- `shape_geometry.c` — the three shapes kept on the CPU: the cube pointed at, the
  capsule and the cylinder built into an arena, and the build any triangles go
  through, whose edges are found by welding by position and walking twice.
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
- `pick.c` — the pixel's ray from the view's two matrices inverted, and the walk that tests
  it against each shape's and loaded model's triangles, then the cameras' boxes and suns' cubes.
- `outline.c` — the walk over one shape's or loaded model's edges that keeps the ones the eye
  is on two sides of, and the quad each of them becomes, a half width per vertex from that
  vertex's own depth.
- `camera_marker.c` — the marker's twenty edges in the camera's own space, handed to marker_lines.c
  for their quads and the slab test against the box.
- `sun_marker.c` — the sun's circle, shaft and head in its own space under its pose unscaled, and
  its cube's hit.
- `point_light_marker.c` — a point light's three circles of 12 in world axes about its position,
  and its world-axis cube's hit.
- `marker_lines.h` — the markers' shared line quads and box slab test, and why they are outline.h's
  quads; internal.
- `marker_lines.c` — each segment's quad with a half width per end and its winding towards the eye,
  and the slab test.
- `collider_marker.c` — each kind's segments in the shape's own space, a circle of 24, handed to
  marker_lines.c for their quads.
- `gizmo.c` — the shaft that covers the same pixels at any distance, the ray against each handle
  nearest first, the point a drag is measured from, and the two meshes of camera-facing quads it is
  all drawn as.
- `gizmo_rings.c` — each ring's plane crossed by the ray, its distance against the radius and its
  angle about the axis, and a camera-facing quad per segment.
- `gizmo_quads.h` — the camera-facing quads both gizmos are built from, a build that knows its
  room; internal.
- `gizmo_quads.c` — a build's arrays in the arena, each normal towards the eye, each triangle wound
  to face it, and a quad as two of them.
- `depth_sort.c` — the stable bottom-up merge sort through the caller's scratch, where the sign
  is the whole of it.
- `draw_system.c` — the camera's view and the sun, and the run: the walk over meshes, model
  parts from the frame's store and panels, the world's solids drawn as found, the held-back groups and the marks in their order.
- `draw_shadows.c` — the sun's shadow passes: the cascades fitted to the frame, and every caster,
  mesh or model part, drawn into each, then the bounce pass when the light bounces, and why this is
  its own call and who casts.
- `draw_point_lights.c` — the point light table into a pass's lights: each placed one at the
  frame's lag about the eye, colour times intensity, falloff as authored, those of intensity 0 and
  those past 256 left out.
- `draw_bounce.h` — the sun's bounce pass, this step's stale spheres and the cascades' caster walk
  it shares; internal.
- `draw_bounce.c` — run only for a light with bounces: the grid fitted, the casters drawn into the
  bounce map, a caster moved between lag 1 and lag 0 marked at both places, and the frame's
  target's grid updated.
- `draw_group.h` — the drawables held back until their turn, and the four groups; internal.
- `draw_group.c` — a group's room in the arena, an entry held with its depth key, the draws sorted
  or in table order, and the record a mesh is drawn with.
- `draw_particles.h` — every live particle as one blended world draw, counted for the group's room
  and held in it; internal.
- `draw_particles.c` — the emitter's picture from the frame's store, the camera-facing matrix
  about the eye, and size, colour and alpha lerped at age over life.
- `draw_water.h` — every water as one blended world draw, counted for the group's room and held
  in it, why the quad is turned and why it casts no shadow; internal.
- `draw_water.c` — the eye-relative matrix times the turned, scaled quad, and the row's colour,
  waves at its clock and sky in the object record.
- `draw_marks.h` — the editor's marks over the world and why each has its own depth; internal.
- `draw_marks.c` — the camera and sun markers, the outline, a collider's lines and the gizmo's
  arrows or rings, each as transient quads.
- `model_upload.h` — a read model's pictures and materials uploaded in one call, every id
  listed, shared by the import and the model store; internal.
- `model_upload.c` — each picture once per colour space wanted, a record per material and
  one default shared by the primitives naming none.
- `import.c` — the model's upload, its geometry and the tree walk that turns a read model
  into one entity per drawn primitive.
- `model_bake.h` — a read model's nodes baked into its vertices and its primitives merged
  into one part per material; internal.
- `model_bake.c` — the walk into a list of placed primitives, each part's room counted
  from it, the fill, and the mirrored node's turned triangles.
- `models.c` — the store's table of entries with an arena each, and the load that reads,
  bakes, uploads and gives back what a failure made; pictures on the quad, the dot and the water apart.
- `model_picture.h` — a picture's quad, decode by extension, the soft dot and the upload of one
  texture and two blended materials; internal.
- `model_picture.c` — the quad's leaning normals, the smoothstep dot, and the lit and glow
  materials on one COLOUR texture.
