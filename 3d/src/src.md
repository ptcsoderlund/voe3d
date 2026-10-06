# src

`3d`'s implementation: one file per public header, plus the built-in shapes' geometry. Nothing
here is included from outside the folder — `include/3d/` is the whole public surface.

- `projection.c` — the projection arithmetic, written out, because the signs are
  the whole thing, and the render view a pose and a lens become.
- `shadow_cascades.c` — the splits, each slice's sphere from the inverted
  projection, and each cascade's snapped light view and box.
- `light_box.h` — the sun's basis, the snap to whole texels in double and the
  look from the sun, used by the cascades; internal.
- `light_box.c` — those three, moved out of shadow_cascades.c unchanged.
- `bounce_grid.c` — the smallest power-of-two spacing whose grid about the box
  centre's cell holds the box with a cell spare, its corner about the eye, and
  the relight's sun view of the volume.
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
- `bounds.c` — one float box about the first counted entity's double position, grown by every
  shape's and loaded model's vertices on the tree, and the framing distance from the half angle.
- `outline.c` — the walk over one shape's or loaded model's edges that keeps the ones the eye
  is on two sides of, and the quad each of them becomes, a half width per vertex from that
  vertex's own depth.
- `camera_marker.c` — the marker's twenty edges in the camera's own space, handed to marker_lines.c
  for their quads and the slab test against the box.
- `sun_marker.c` — the sun's circle, shaft and head in its own space under its pose unscaled, and
  its cube's hit.
- `point_light_marker.c` — a point light's three circles of 12 in world axes about its position,
  and its world-axis cube's hit.
- `place_marker.c` — a place's octahedron of 12 edges in world axes about its position, its
  world-axis cube's hit, and the eight tables an entity must have no row in to wear it.
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
- `draw_system.c` — the camera's view, the first light row, the frame's pass camera, and the run
  over meshes, model parts and panels: solids drawn as found, fading models held blended, then the
  held-back groups and the marks.
- `draw_shadows.c` — the shadow passes as one call: each casting directional light's cascades in
  its own slot of only the casters its blockers hold, the point-shadow pass and the probe bounce,
  a gone model casting nothing.
- `draw_point_lights.c` — the point light table into a pass's lights: each placed one at the
  frame's lag about the eye, colour times intensity, falloff as authored, those of intensity 0 and
  those past 256 left out, then the nearest 16 casting ones slotted and faded by the 17th.
- `light_blocker.c` — a blocker's row and transform into a box: the world place at the lag, its
  rotation, and |scale| × size / 2 per axis.
- `draw_light_blockers.c` — the light blocker table into a pass's blockers: each placed box about
  the eye with its rows and sphere, flat ones and those past 32 left out, the Direct and Fill bits,
  the sun's mask, and light rows 2 to 4 each masked at its place.
- `draw_bounce.h` — the frame's probe bounce, this step's stale spheres, the still casters' box
  and the cascades' caster walk and casting test it shares; internal.
- `draw_bounce.c` — run only when a light bounces: the volume fitted to the still casters' world
  box, the bounce begun with every sun, lamp, blocker and stale sphere, the casters drawn into each
  capture pass and casting sun's map, and the relight.
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
- `draw_marks.h` — the editor's marks over the world, bare places' and every blocker's among them,
  and why each has its own depth; internal.
- `draw_marks.c` — the camera's marker, every sun's, lamp's and bare place's, every blocker's lines,
  the outline, a collider's and the selected blocker's lines, written once, and the gizmo's arrows
  or rings, each as transient quads.
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
- `models.c` — the store's table of entries with an arena each, and the load that reads, bakes,
  uploads with each part's blended twin and gives back what a failure made; pictures on the quad,
  the dot and the water apart.
- `model_picture.h` — a picture's quad, decode by extension, the soft dot and the upload of one
  texture and two blended materials; internal.
- `model_picture.c` — the quad's leaning normals, the smoothstep dot, and the lit and glow
  materials on one COLOUR texture, blended already so needing no twin.
