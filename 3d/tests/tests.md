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
- `bounce_grid.c` — that the grid stands 32 m ahead in whole cells, its corner
  is its cell about the eye 10 km out, 2 m along x is one cell, its corners lie
  in the light box and a 1 cm move keeps the origin on its 8-texel block. Needs
  no graphics card.
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
  frames blind, a red shape reads red, a gizmo's arrows and rings show through a cube and a camera
  or sun marker is one draw more. Skips without a graphics card.
- `draw_particles.c` — that a burst of five with the dot loaded is five draws more than no emitter,
  none with no store, and that a glowing one is unlit. Skips without a graphics card.
- `draw_water.c` — that a 4 × 4 water over a ground cube changes the picture's centre, clocks 0
  and 1.3 differ, a store without the record draws none, and the shadow passes draw the same with
  or without it. Skips without a graphics card.
- `point_lights.c` — that a frame's point lights are the table's about the eye, colour times
  intensity with the authored falloff, dark and unplaced ones left out, a parented one carried,
  and that one lights the ground under it and not a far corner. Skips without a graphics card.
- `shadows.c` — that a cube under a sun straight down shadows the floor beneath it, a fill lifts
  the shadow, no light casts nothing, and the same holds 100 km out, both pictures with the bounce
  off, and for a model. Skips without a graphics card.
- `bounce.c` — the shadows call's pass count with and without bounces, and the stale spheres a
  moved wall marks. Skips without a graphics card.
- `bounce_scene.c` — bug 01 through the editor's calls at sun 1 and π: the red box tints nearby
  ground while its own shadow stays faint and no ground darkens. Skips without a graphics card.
- `no_light.c` — that a world with no light frames a zeroed light, drawn black and not blind, and
  that one light frames as itself, its direction its transform's -Z and its fill colour times
  strength. Needs no graphics card.
- `shape_geometry.c` — that the CPU store answers for the three kinds and nothing else, holds each
  kind's own triangles, that every kind is a closed surface wound counter-clockwise seen from
  outside, and that the cube's arrays built alone give its edge count. Needs no graphics card.
- `pick.c` — that the pick ray meets the nearest cube, camera box, sun cube, model or child at the
  right distance and nothing where nothing is, and that a drawn pixel the cube covers picks it.
- `outline.c` — the silhouette edge counts, the quads' corners, width and winding for cubes, children
  and models, and that a hidden cube's outline shows through when drawn.
- `camera_marker.c` — the marker's twenty edges' worth of geometry, a camera scaled to nothing
  building nothing and not hit, and a ray meeting the box square on, turned and not at all. Needs
  no graphics card.
- `sun_marker.c` — the marker's twenty-nine edges, a turned sun's arrow tip a metre along its
  turned -Z, a scaled sun building the same quads, and a ray meeting the cube square on, turned and
  not at all. Needs no graphics card.
- `point_light_marker.c` — the marker's thirty-six edges seen off-axis, no area building nothing,
  a ray meeting the cube 0.25 m short and missing beside it, and a pick answering a lamp and the
  nearer of a lamp and a cube. Needs no graphics card.
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
- `shape.c` — the shape table's description, default row, intents and runs, each kind's own
  geometry, and the upload's two material records. The table and geometry half needs no graphics
  card; the upload half skips without one.
- `model_component.c` — the model's one CHAR field `path` of 128, a submitted path read back after a
  run, a dead entity's intent dropped and a path with no end cut. Needs no graphics card.
- `emitter_component.c` — an added emitter's fields read back, the default row as 0298 says,
  the particles runtime-only, and both submits taken. Needs no graphics card.
- `emitter_system.c` — a rate's count over a second, a burst on a stopped emitter, stop, rise,
  death after life, the cap of 64 and a removed emitter's row gone. Needs no graphics card.
- `water_component.c` — both tables registered, the waves runtime-only, the default row as 0305
  says, every field named with its kind, and a replace queued. Needs no graphics card.
- `water_system.c` — a waves row after one run, the clock stepped and wrapped at 60 s, a replace's
  colour, and a removed water's row gone. Needs no graphics card.
- `models.c` — the model store: loads, failures kept as failed, replace, clear, pictures and
  the uncounted dot, and a model drawn only with the store. Skips without a card.
