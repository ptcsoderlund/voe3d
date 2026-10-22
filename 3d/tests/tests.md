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
- `bounce_grid.c` — that the probe volume fits the level's box, not the eye, the relight's sun view
  holds it, and a nest about the eye starts, holds still, steps a cell and jumps home as the eye
  moves. Needs no graphics card.
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
- `draw_material.c` — that a cube wears an unlit material's colour, a model's part wears what its
  parts set names, and a double-sided material draws both sides. Skips without a graphics card.
- `draw_gizmo.c` — that a gizmo's arrows and rings show through the cube it stands in. Skips
  without a graphics card.
- `draw_markers.c` — that camera, sun, place and brush markers each add the draws they should
  and a zeroed marker none, and that a ray picks each sun. Skips without a graphics card.
- `draw_particles.c` — that a burst of five with the dot loaded is five draws more than no emitter,
  none with no store, and that a glowing one is unlit. Skips without a graphics card.
- `draw_water.c` — that a 4 × 4 water over a ground cube changes the picture's centre, clocks 0
  and 1.3 differ, a store without the record draws none, and the shadow passes draw the same with
  or without it. Skips without a graphics card.
- `draw_model_fade.c` — that a model on a ground cube seen from above changes the centre at fade 0
  and 0.5, equals no model row at 1, and casts at 0.5 but not at 1. Skips without a graphics card.
- `draw_levels.c` — that a levelled sphere 400 m away draws under a quarter of the triangles it
  draws 3 m away, one loaded without levels draws whole, and in level 1's band it dissolves into
  level 0 unless fading. Skips without a graphics card.
- `draw_foliage.c` — that a landscape's copies of a model gather into groups summing to three per
  opaque part, none without the row, kind or room, and that their shadow shows and three draw as
  many draws as one. Skips without a card.
- `draw_switched_off.c` — that a sphere on a child draws as no model row does when the child or
  its parent is off, and whole again when both are back on. Skips without a graphics card.
- `draw_terrain.c` — that a 4096 m landscape seen level from 2 m up draws its far ridge and ground
  within the node budgets, and a painted one draws its layers. Skips without a graphics card.
- `terrain_shadow.c` — that a 512 m landscape's 40 m ridge, under a low sun behind it, darkens
  the ground behind it and not the ground in front, in one frame, its nodes drawn into each
  cascade. Skips without a graphics card.
- `point_lights.c` — that a frame's point lights are the table's about the eye with their bounces,
  the nearest 16 casting lamps take the shadow slots, and one lights the ground under it. The
  picture skips without a graphics card.
- `light_blockers.c` — that a blocker's box follows its transform, a frame records it with its
  block's bit and the sun's mask, and the editor lines it in the selected or rest colour.
- `directional_lights.c` — that one light frames no more, a moon after a sun rides in `more_lights`
  with its light, bounces and strength, five lights keep three more, a Room about the moon sets its
  mask and not the sun's, and the pass camera carries `more`. Needs no graphics card.
- `lights_switched_off.c` — that an off sun or moon keeps its row with no light and casts not, and
  an off lamp or blocker is left out of the frame. Needs no graphics card.
- `shadows.c` — that a cube under a straight-down sun shadows the floor, near the origin and
  100 km out, and that a casting lamp beside a cube darkens the floor on the cube's far side.
  Skips without a graphics card.
- `shadow_lights.c` — that a sun and a moon each cast their own shadow, a sun not casting leaves
  the moon's, one casting light fits one light's passes, a moon bounces, and each bouncing sun
  draws its own map per volume. Skips without a graphics card.
- `blocked_shadows.c` — that a moon inside an All, Direct or Fill box, or alone inside an All, is
  not shadowed by a roof outside it yet casts the cube inside, and that with no box the roof shadows
  the floor. Skips without a graphics card.
- `bounce.c` — the shadows call's passes over two frames, two views each drawing theirs, the
  nests begun and captured, the caster-sized stale spheres and the still casters' box. Skips
  without a card.
- `bounce_scene.c` — that the probe bounce as the editor draws it tints, settles and stays put
  under turns, moves and a blocker. Skips without a card.
- `bounce_tint.c` — that a small box tints the ground beside it through strength, colour, moves,
  a whole relight and removal. Skips without a card.
- `bounce_pacing.c` — that an eye flying 0.15 m a frame opens one capture pass a frame and fits
  that room every frame, then settles once still. Skips without a card.
- `bounce_foliage.c` — that a kind set relights the bounce, an erase does too, and the relit sun
  maps draw a whole multiple of the foliage groups more with copies than without. Skips without a
  card.
- `bounce_world.inc` — the bounce tests' harness: the floor, sun and placed box, the reference at
  bounces 0, one frame through the shadows call, the settle, the window read and a point's pixel.
- `no_light.c` — that a world with no light frames a zeroed light, drawn black and not blind, and
  that one light frames as itself, its direction its transform's -Z and its fill colour times
  strength. Needs no graphics card.
- `sky.c` — that a frame's sky and picture are zeroed with no table or row and are the first rows,
  the haze and its top the Haze row's, each light's disc and glow carried, and a sky shows behind
  a lit slab. The picture skips without a graphics card.
- `camera_switched_off.c` — that a camera off, itself or through a parent, frames from the
  origin with the table's default row and no lag, and as itself when on (0441). Needs no
  graphics card.
- `sky_switched_off.c` — that a Sky, Haze or Picture row whose entity or parent is off is as no
  row, back when on, and an off first Haze row gives the second's. Needs no graphics card.
- `shape_geometry.c` — that the CPU store answers for its three kinds alone, each a closed surface
  wound counter-clockwise from outside, that flat triangles give no edges, and that a jittered
  million-vertex grid welds in linear time. Needs no graphics card.
- `pick.c` — that the pick ray meets the nearest cube, model, water plane or child at the right
  distance and nothing where nothing is, a camera, sun or place marker wins over a cube in front of
  it or behind it, a drawn pixel the cube covers picks it, and passes what is off.
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
- `gizmo.c` — rays meeting each arrow and square and missing beside them, the shaft growing with
  distance and absent behind or beside the eye, the grabs and the refusal, and the meshes' counts,
  winding and marking. Needs no graphics card.
- `gizmo_rings.c` — a ray on each rim, through the centre and across two rings, a quarter turn
  about Y reading π/2, a ray in the plane refused, 100 km out the same, and the meshes' counts,
  winding and marked ring. Needs no graphics card.
- `far.c` — that everything moved 100 km out picks the same cube at the same distance, grabs a
  millimetre as a millimetre, and frames about the camera's double position. Needs no graphics
  card.
- `shape.c` — the shape table's description, default row, intents and runs, each kind's own
  geometry, the changes table and the upload's material records. The table and geometry half needs
  no graphics card; the upload half skips without one.
- `model_component.c` — the model component's fields, its default and intents read back after a
  run, a dead entity's intent dropped and a long path cut. Needs no graphics card.
- `emitter_component.c` — an added emitter's fields read back, the default row as 0298 says,
  the particles runtime-only, and both submits taken. Needs no graphics card.
- `emitter_system.c` — a rate's count over a second, a burst on a stopped emitter, stop, rise,
  death after life, the cap of 64, a removed emitter's row gone, and an off emitter gone and new
  again. Needs no graphics card.
- `water_component.c` — both tables registered, the waves runtime-only, the default row as 0305
  says, every field named with its kind, and a replace queued. Needs no graphics card.
- `water_system.c` — a waves row after one run, the clock stepped and wrapped at 60 s, a replace's
  colour, and a removed water's row gone. Needs no graphics card.
- `model_bake.c` — that the bake carries a file's vertex colours as complements and bakes one
  `_LOD<n>` level of a file's own at a time. Needs no graphics card.
- `models.c` — the model store's loads, failures, replaces, clears and blended twins, glTF source
  and `wears`, pictures, a model drawn only with the store and a 17-material refusal.
  Skips without a card.
- `models_levels.c` — the store's levels: a levels file's and a file's own `_LOD1`, a far swap,
  none leaked across reloads and clears, pools below level 0, a prepare then a put, and 17
  materials refused. Skips without a card.
- `models_landscape.c` — a `.landscape` loaded as one part on the shared grid, its brushes, puts,
  layers and renames written within a frame's budget, and its foliage placed, kept standing by a
  sculpt, scattered, erased, put back, kinds set, sized and renamed. Skips without a card.
- `models_material.c` — a `.material` with four maps loaded as one part, its cutout, emission and
  sides, a set written by a frame, a decoded put alike, a non-picture map MALFORMED, and a rename.
  Skips without a card.
- `landscape.c` — a bilinear height, a ray's hit and miss, raise, smooth and flatten's rates, and
  painting a layer: the sum kept, the radius held and a light stroke stopping at its strength.
  Needs no graphics card.
- `foliage.c` — a held scatter stamp filling to about its density, spaced, picked and within its
  radius; a seed repeating; erase by kind and undo's put both ways; a copy leaning 45° or upright on
  a slope; a replace following a raised height; a kind's base size. Needs no graphics card.
- `landscape_lod.c` — on 2048 cells of hills, eyes on a lattice choosing nodes that cover the square
  once, neighbours within a level, within capacity, bounding their heights; an update equal to a
  fresh build and the pyramid's box. Needs no graphics card.
- `simplify.c` — a flat grid to an eighth at no error with its corners, a sphere to a half facing
  out, the sphere held by a tiny limit, a uv seam moving along its line untorn, and a faceted
  cylinder cut along its seam lines at angle 0 and further softened at 180. Needs no graphics card.
- `model_levels.c` — a sphere's levels file made and read back as asked,
  its own levels measured,
  going stale when its `.glb` or version changes, each level's triangles counted, and a view's level
  and pixel share chosen by its screen size; needs no graphics card.
- `measure.c` — that two hand-built bakes measure 0 alike, a lift's height apart, √2 for a big
  quad's far corner either way, and 0.5 for one vertex lifted in a 32 × 32 grid. Needs no
  graphics card.
- `model_glb.inc` — a UV sphere's `.glb` built on an arena, with a second "Ball_LOD1" root wearing
  half its triangles when asked.
