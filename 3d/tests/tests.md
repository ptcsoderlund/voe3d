# tests

`3d`'s own tests: plain C programs with an ordinary `main()`, zero for pass, found by the build and
registered nowhere. A reader is here to find which file already makes a claim before making it
again, or to find where a claim that has started failing is written down.

- `projection.c` — reversed depth in the lens and the box, the aspect ratio, that nothing in the
  projection flips Y, and the view of a pose and a lens, a flattened pose seeing
  nothing. Needs no graphics card.
- `shadow_cascades.c` — that the splits rise to the reach, each slice lies in
  its cascade's box, and a moved and turned eye moves the map by whole texels,
  near the origin and 100 km out. Needs no graphics card.
- `bounce_grid.c` — that the probe volume fits the level's box, not the eye: the same cell and
  spacing from three eyes, a centre across a cell edge one cell on, no box about the origin, and a
  sun view that holds the volume and grows with the spacing. Needs no graphics card.
- `depth_sort.c` — that the order visits the furthest away first, and that equal
  depths keep the order they came in. Needs no graphics card.
- `normal_matrix.c` — that a normal stays perpendicular to a non-uniformly scaled
  surface and that the world matrix's answer does not. Needs no graphics card.
- `panel.c` — the panel table, and that a panel is sorted among the see-through
  meshes rather than drawn in a pass of its own. The table half needs no graphics
  card; the ordering half skips without one.
- `material.c` — that a material saying nothing about its UV rect reads its whole
  texture, and that one saying something keeps it. Skips without a graphics card.
- `import.c` — a hand-built `.glb` in and entities out: the flattening, the
  shared picture, the mirrored model, and one frame drawn from what it made.
  Skips without a graphics card.
- `model_data.inc` — that hand-built `.glb`, as bytes.
- `draw_system.c` — that a hidden entity is exactly the one not drawn, a camera scaled to nothing
  frames blind, the eye lags the last step and a red shape reads red. Skips without a graphics card.
- `draw_gizmo.c` — that a gizmo's arrows and rings show through the cube it stands in. Skips
  without a graphics card.
- `draw_markers.c` — that a marked camera or sun is one draw more and a zeroed marker none, that
  two suns are one draw and one selected two, that a ray picks each sun, and that a bare place
  is marked and one with a shape not. Skips without a graphics card.
- `draw_particles.c` — that a burst of five with the dot loaded is five draws more than no emitter,
  none with no store, and that a glowing one is unlit. Skips without a graphics card.
- `draw_water.c` — that a 4 × 4 water over a ground cube changes the picture's centre, clocks 0
  and 1.3 differ, a store without the record draws none, and the shadow passes draw the same with
  or without it. Skips without a graphics card.
- `draw_model_fade.c` — that a model on a ground cube seen from above changes the centre at fade 0
  and 0.5, equals no model row at 1, and casts at 0.5 but not at 1. Skips without a graphics card.
- `point_lights.c` — that a frame's point lights are the table's about the eye with their bounces,
  the nearest 16 casting lamps take the shadow slots, and one lights the ground under it. The
  picture skips without a graphics card.
- `light_blockers.c` — that a blocker's box follows its transform, a frame records it with its
  block's bit and the sun's mask, and the editor lines it in the selected or rest colour.
- `directional_lights.c` — that one light frames no more, a moon after a sun rides in `more_lights`
  with its light, bounces and strength, five lights keep three more, a Room about the moon sets its
  mask and not the sun's, and the pass camera carries `more`. Needs no graphics card.
- `shadows.c` — that a cube under a straight-down sun shadows the floor, near the origin and
  100 km out, and that a casting lamp beside a cube darkens the floor on the cube's far side.
  Skips without a graphics card.
- `shadow_lights.c` — that a sun and a moon each cast their own shadow on the floor from the second
  frame, a sun that does not cast leaves only the moon's, and one casting light fits one light's
  passes without growing the array. Skips without a graphics card.
- `blocked_shadows.c` — that a moon inside an All, Direct or Fill box, or alone inside an All, is
  not shadowed by a roof outside it yet casts the cube inside, and that with no box the roof shadows
  the floor. Skips without a graphics card.
- `bounce.c` — the shadows call's passes over two frames, captures and sun map for a sun or lamp
  that bounces, two views in one frame each drawing their sun map, the stale spheres a moved wall
  marks, and the still casters' box from two eyes and a turned one. Skips without a card.
- `bounce_scene.c` — the probe bounce through the editor's and the game's calls: a red box tints the
  ground it faces, open ground stays even, the grid settles, a light blocker keeps the tint out of
  its patch, and camera turns and moves change nothing, nor does a far eye. Skips without a card.
- `no_light.c` — that a world with no light frames a zeroed light, drawn black and not blind, and
  that one light frames as itself, its direction its transform's -Z and its fill colour times
  strength. Needs no graphics card.
- `shape_geometry.c` — that the CPU store answers for the three kinds and nothing else, holds each
  kind's own triangles, that every kind is a closed surface wound counter-clockwise seen from
  outside, and that the cube's arrays built alone give its edge count. Needs no graphics card.
- `pick.c` — that the pick ray meets the nearest cube, model, water plane or child at the right
  distance and nothing where nothing is, a camera, sun or place marker wins over a cube in front of
  it or behind it, and a drawn pixel the cube covers picks it.
- `bounds.c` — a scaled cube's centre and radius, a parent's box from its child and round a far
  child, a bare transform with none, a cube 100 km out to the millimetre, and the framing distance
  √2 for a square picture and more for a tall one. Needs no graphics card.
- `outline.c` — the silhouette edge counts, the quads' corners, width and winding for cubes,
  children and models, and that a hidden cube's outline shows through when drawn.
- `camera_marker.c` — the marker's twenty edges' worth of geometry, a camera scaled to nothing
  building nothing and not hit, and a ray meeting the box square on, turned and not at all. Needs
  no graphics card.
- `sun_marker.c` — the marker's twenty-nine edges, a turned sun's arrow tip a metre along its
  turned -Z, a scaled sun building the same quads, and a ray meeting the cube square on, turned and
  not at all. Needs no graphics card.
- `point_light_marker.c` — the marker's thirty-six edges seen off-axis, no area building nothing,
  a ray meeting the cube 0.25 m short and missing beside it, and a pick answering a lamp alone and
  before or behind a cube, a marker over a mesh. Needs no graphics card.
- `place_marker.c` — the diamond's twelve edges, no area building nothing, a ray meeting the cube
  0.25 m short and missing 0.3 m aside, and who wears it, an unregistered table counting as no
  row. Needs no graphics card.
- `collider_marker.c` — a box's twelve edges, a sphere's and a capsule's counts, a box twice the
  size twice as far out, quads about the eye 100 km out, and the collider that fits each shape.
  Needs no graphics card.
- `gizmo.c` — a ray across each arrow and through each square, the space that meets nothing, the
  shaft doubling with the distance, the two grabs and the one refusal, and the two meshes' counts,
  their winding towards the eye and the handle marking moves across. Needs no graphics card.
- `gizmo_rings.c` — a ray on each rim, through the centre and across two rings, a quarter turn
  about Y reading π/2, a ray in the plane refused, 100 km out the same, and the meshes' counts,
  winding and marked ring. Needs no graphics card.
- `far.c` — that everything moved 100 km out picks the same cube at the same distance, grabs a
  millimetre as a millimetre, and frames about the camera's double position. Needs no graphics
  card.
- `shape.c` — the shape table's description, default row casting, intents (cast_shadows false
  among them) and runs, each kind's own
  geometry, and the upload's two material records. The table and geometry half needs no graphics
  card; the upload half skips without one.
- `model_component.c` — the model component's fields, its default and intents read back after a
  run, a dead entity's intent dropped and a long path cut. Needs no graphics card.
- `emitter_component.c` — an added emitter's fields read back, the default row as 0298 says,
  the particles runtime-only, and both submits taken. Needs no graphics card.
- `emitter_system.c` — a rate's count over a second, a burst on a stopped emitter, stop, rise,
  death after life, the cap of 64 and a removed emitter's row gone. Needs no graphics card.
- `water_component.c` — both tables registered, the waves runtime-only, the default row as 0305
  says, every field named with its kind, and a replace queued. Needs no graphics card.
- `water_system.c` — a waves row after one run, the clock stepped and wrapped at 60 s, a replace's
  colour, and a removed water's row gone. Needs no graphics card.
- `models.c` — the model store: loads, failures kept as failed, replace, clear, each part's
  blended twin and none leaked, pictures and the uncounted dot, and a model drawn only with the
  store. Skips without a card.
- `models_landscape.c` — a `.landscape` loaded as sixteen parts on one material, a brush marking it
  edited and the frame drawing its chunks transient, the settle making them static, a put's rect
  and a rename. Skips without a card.
- `landscape.c` — a bilinear height, a ray's hit and miss, raise, smooth and flatten's rates, a chunk wound up
  and a rect on a chunk edge in both. Needs no graphics card.
