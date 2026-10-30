# src

`3d`'s implementation: one file per public header, plus the built-in shapes' geometry. Nothing
here is included from outside the folder — `include/3d/` is the whole public surface.

- `projection.c` — the projection arithmetic, written out, because the signs are
  the whole thing, and the render view a pose and a lens become.
- `cull.h` — a view's planes and whether a sphere about the eye is inside them, a cascade's sun
  side open, and why a sphere and why here; internal.
- `cull.c` — the six planes from the rows of projection × view, each normalised, the sphere's
  test and its radius under a world matrix.
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
- `model_component.c` — the model table: its key, its registration with a default row not
  blending with ground over 0.5 m, the reads, the intent's submit and the drain that cuts a path
  with no end.
- `emitter_component.c` — the emitter and particles tables: their keys, the registration with
  0298's defaults, the reads and the two submits.
- `emitter_system.c` — the two drains, the particles rows added and dropped, the births in a
  cone through the world matrix from a xorshift, and the step that moves and kills.
- `water_component.c` — the water and waves tables: their keys, the registration with 0305's
  defaults, the reads and the replace submit.
- `water_system.c` — the replace drain, the waves rows added and dropped, and the clock stepped
  in double and wrapped with fmod.
- `shape_system.c` — the one upload of the three shapes' geometry and their two materials, the
  intent's submit and drain, the run that gives, repoints and drops a shape's mesh and material,
  and the opt-in changes table.
- `shape_geometry.c` — the three shapes kept on the CPU: the cube pointed at, the
  capsule and the cylinder built into an arena, and the build any triangles go
  through, whose edges are found by welding by position through hashed cells and walking twice,
  flat triangles skipped.
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
  it against each shape's and loaded model's triangles or a landscape's heights, then the cameras'
  boxes and suns' cubes, and the landscape's own-space hit a brush stamps at.
- `bounds.c` — one float box about the first counted entity's double position, grown by every
  shape's and loaded model's vertices or landscape box's corners on the tree, and the framing
  distance from the half angle.
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
- `brush_marker.c` — the brush's two rings of 48 about its centre in the grid's space, each point
  on the ground plus 5 cm, under the row's world matrix.
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
- `gizmo_scales.c` — the centre square crossed first, then each own axis's segment within the grip,
  the grab on a line or the plane facing the eye, the factor with its floor, and a quad per shaft
  and square.
- `gizmo_snap.c` — the handle's named axes, each rounded in double, a position kept double, an
  angle wrapped by remainder first and a scale's size floored at a step with its sign kept.
- `gizmo_quads.h` — the camera-facing quads all three gizmos are built from, a build that knows
  its room; internal.
- `gizmo_quads.c` — a build's arrays in the arena, each normal towards the eye, each triangle wound
  to face it, and a quad as two of them.
- `depth_sort.c` — the stable bottom-up merge sort through the caller's scratch, where the sign
  is the whole of it.
- `draw_system.c` — the camera's view, the first light, the first on sky, Haze and picture rows,
  the frame's Clouds and Fog volume rows, its pass camera, and each drawable's draw, foliage
  after the landscapes, the clouds then the fog after the blended draws, each mark in turn.
- `draw_clouds.c` — the first Clouds row that is on, as render's record: its kind mapped, its bottom above
  the eye and the eye less the drift wrapped into the period, both in double.
- `draw_models.h` — the model rows' walk, taking the frame's grounds, its part count for the
  blended group's room and the entry and landscape test the landscape walk shares; internal.
- `draw_models.c` — each model row's parts at the LOD the frame's memory keeps, solid drawn now,
  dissolving from the LOD before a swap, blended or fading ones held, unseen rows counted, and a row blending with the
  ground filled with it, its solid parts last in the `ground blend` span.
- `draw_ground.h` — a frame's painted grounds, the one under a point and a part's record filled
  with one; internal.
- `draw_ground.c` — the painted landscape rows the run draws solid as grounds, each with its paint,
  layers and inverse place about the eye, the first whose square holds a point, and the fill with
  its reach clamped.
- `draw_foliage.h` — the key a foliage copy's group is found by, its ground among it, the groups'
  solid draws, blending ones in a `foliage blend` span, and cast draws, and why rest groups and a
  shadowless kind's do not cast; internal.
- `draw_foliage.c` — the foliage gathering: each drawn landscape row's own then grown copies given
  tier then a kept or, unseen, stateless level, a blending kind's keyed by its row's ground,
  written as instances in one call;
  then each group's instanced draw.
- `draw_terrain.h` — a landscape row's chosen nodes as records on the shared grid, drawn solid or
  held blended unless outside the pass's planes, and why one eye chooses the same nodes in
  every pass and the cull comes after; internal.
- `draw_terrain.c` — the eye taken into the grid's space, the nodes selected on scratch or, in
  the cast, on the stack, each node's record over the heights texture, its layers looked up
  once per row per call, and its box's sphere tested and counted.
- `draw_shadows.c` — the shadow passes as one call: each casting sun's cascades in its slot, of
  what its blockers hold and its sides see, counted, the point-shadow pass and the bounce, a
  landscape by the view's nodes, foliage in free suns' cascades, each part timed by any clock.
- `draw_point_lights.c` — the point light table into a pass's lights: each placed one at the
  frame's lag about the eye, colour times intensity, falloff as authored, those of intensity 0, an
  off owner's and those past 256 left out, then the nearest 16 casting ones slotted and faded by the 17th.
- `light_blocker.c` — a blocker's row and transform into a box: the world place at the lag, its
  rotation, and |scale| × size / 2 per axis.
- `fog_volume.c` — a fog volume's box as a blocker's is, and the table into render's records: each
  kept box's rows about the eye, the squared density, and the patches and change wrapped in double.
- `draw_light_blockers.c` — the light blocker table into a pass's blockers: each placed box about
  the eye with its rows and sphere, flat ones, an off owner's and those past 32 left out, the Direct and Fill bits,
  the sun's mask, and light rows 2 to 4 each masked at its place.
- `draw_bounce.h` — the frame's probe bounce with foliage in free suns' maps, its stale and
  removed casters' spheres, the caster walk and measure bounce_casters.c shares, the still casters'
  box, and the cascades' culled caster walk, casting test and clock helpers; internal.
- `bounce_casters.c` — a removed caster's sphere where the target's memory saw it last, then the
  memory refilled with this frame's casters.
- `draw_bounce.c` — the frame's probe bounce, when a light bounces: the volumes fitted to the
  still casters, begun with the store's count, the casters drawn into each pass and sun map, the
  foliage into a free sun's map, a landscape coarse at 16 nodes, the relight; six parts timed.
- `draw_group.h` — the drawables held back until their turn, and the four groups; internal.
- `draw_group.c` — a group's room in the arena, an entry held with its depth key, the draws sorted
  or in table order, the record a mesh is drawn with, a double-sided mesh drawn between two
  two-sided calls, and the left-out test: the frame's hidden entity or a switched-off owner.
- `draw_material.h` — the loaded material a shape's or model part's worn path names in the frame's
  store, for its record; internal.
- `draw_material.c` — the empty path refused, then the store's linear find.
- `draw_particles.h` — every live particle as one blended world draw, counted for the group's room
  and held in it unless the camera cannot see it; internal.
- `draw_particles.c` — the emitter's picture from the frame's store, the camera-facing matrix
  about the eye, size, colour and alpha lerped at age over life, and the quad's sphere tested.
- `draw_water.h` — every water as one blended world draw, counted for the group's room and held
  in it unless the camera cannot see it, why the quad is turned, why its sphere holds the wave
  height and why it casts no shadow; internal.
- `draw_water.c` — the eye-relative matrix times the turned, scaled quad, the row's colour,
  waves at its clock and sky in the object record, and the quad's sphere tested.
- `draw_marks.h` — the editor's marks over the world, bare places', the brush's, every
  blocker's and fog volume's and the gizmo's arrows, rings or scales among them, and why each has
  its own depth; internal.
- `draw_marks.c` — the camera's, every sun's, lamp's and place's markers, the brush's rings,
  every blocker's and fog volume's box lines from one builder, the outline, a collider's and the
  selected box's lines, and the gizmo by its kind, each as transient quads.
- `model_upload.h` — a read model's pictures and materials uploaded in one call, every id
  listed, shared by the import and the model store; internal.
- `model_upload.c` — each picture once per colour space wanted, a record per material and
  one default shared by the primitives naming none.
- `picture_upload.h` — one picture as one SMOOTH texture, blocks, an RGBA8 chain or one level the
  GPU mips, shared by a model's pictures and a material's maps; internal.
- `picture_upload.c` — the picture's format into `render`'s blocks, and the create of its shape.
- `import.c` — the model's upload, its geometry and the tree walk that turns a read model
  into one entity per drawn primitive, a full world and a deep nesting said as limits.
- `model_bake.h` — a read model's nodes of one level baked into its vertices, its vertex colours
  carried, and its primitives merged into one part per material, and the count of the file's own
  levels its node names tell; internal.
- `model_bake.c` — the walk into a list of placed primitives, each part's room counted
  from it, the fill with each colour's complement, the mirrored node's turned triangles, and
  each node's `_LOD<n>` level carried down the walk; its three refusals as limits with the file's
  own counts.
- `models.c` — the store's table of entries with an arena each, the `.glb` load as a prepare and
  a put that uploads each part with its blended twin and gives back what a failure made; the parts
  set; pictures on the quad, the dot and the water apart.
- `models_store.h` — the store's internal table with its count of changes the bounce relights
  by, a landscape's grown copies beside the file's, the prepared `.glb` and the helpers the
  store's files share.
- `model_prepare.c` — a read `.glb` prepared off the device: level 0 and own levels baked until
  one matches no part, or a levels file read and checked against the bake, the shape built and
  the sphere measured.
- `model_cooked.c` — a prepared `.glb` measured then written as one run of 16-aligned sections,
  each picture mipped by its first use, and read back pointing into the bytes, every count, range
  and index checked.
- `models_levels.c` — a `.glb`'s levels beside its parts, as the prepare baked or read them: own
  levels joined by material, or made levels' indices shared over level 0's vertices, each bounded
  through voe_3d_model_levels_bounds; the level-0 mirror and the free.
- `models_landscape.c` — a landscape's grid copied with its layers, paint, foliage kinds and
  copies, each placed, its grown copies made, uploaded as heights, paint and a pyramid; brushes
  and puts growing its dirt, placing copies again and remaking the grown; renames following.
- `models_foliage.c` — the store's foliage calls: a loaded landscape's grid and placed transforms
  handed to foliage.c, a kind's look, its blend counted, or a layer's growth set remaking that
  kind's copies, the entry marked edited and the store's count raised on a change.
- `models_material.c` — a `.material`'s four maps, decoded by the load under each map's name as
  the subject or already by the caller, put mipped with its record, cutout, emission and sides
  and its twin as one part with no geometry; the set marking it and the frame rewriting both records.
- `model_picture.h` — a picture's quad, the soft dot and the upload of one texture and two
  blended materials, the public decode by extension included; internal.
- `model_picture.c` — the decode by extension, the quad's leaning normals, the smoothstep dot,
  and the lit and glow materials on one COLOUR texture, blended already so needing no twin.
- `landscape.c` — the bilinear height, the marched and bisected ray, and the brush's stamp on the
  heights or, painting, on a layer's weights.
- `brush_weight.h` — a brush's soft weight by radius and softness, shared by the sculpt, paint and
  foliage stamps; internal.
- `foliage.c` — a copy placed on the bilinear ground's normal at its kind's base size, a scatter
  stamp's tries drawn up front with the spreads of each one's kind and refused against the near
  copies in one walk, erase and undo by moving the last in, and the replace over a rect.
- `foliage_grow.c` — the area's grown copies of the remade kinds removed, then each kind's
  lattice cells over it hashed, jittered, kept by the bilinear paint and placed.
- `landscape_lod.c` — the min/max pyramid built and updated over a rect, its root's box, and the
  selection splitting nodes coarse to fine by the eye's distance within a capacity.
- `simplify.h` — a part's triangles made fewer by half-edge collapse under quadric error, borders
  held, seam lines moved along their line and normals within an angle softened, for a model's
  levels; internal.
- `simplify.c` — the weld, each vertex's side, the plane sums and a heap of stamped collapses
  popped cheapest first, a seam line's re-pointed by side and a soft position's onto the kept
  vertex, each refused past 90° of turn.
- `model_levels.c` — a `.glb`'s levels file made with no picture decoded (simplified level by
  level, or own levels measured), read back with stale fields refused, its swaps patched in
  place; each level's triangles, the bounds capped and spread, and a bound's metres.
- `lods_kept.c` — a view's kept LODs: two stamped tables probed from a mixed key, this frame's
  filled and last frame's read, the swap rule over model_levels.c's size and level, and the keys.
- `measure.h` — how far apart two bakes are, both ways, vertex to nearest triangle, for an own
  level's error; internal.
- `measure.c` — each way's triangles in a grid of about one a cell, searched ring by ring out
  from each vertex, and Ericson's closest point on a triangle.
