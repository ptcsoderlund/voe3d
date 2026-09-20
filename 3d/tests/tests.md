# tests

`3d`'s own tests: plain C programs with an ordinary `main()`, zero for pass,
found by the build and registered nowhere. A reader is here to find which file
already makes a claim before making it again, or to find where a claim that has
started failing is written down.

The matrices and the sort are arithmetic and their tests need nothing on the
machine; everything that measures a picture or a draw count needs a graphics
card, and says so below.

- `projection.c` — reversed depth, the aspect ratio, and that nothing in the
  projection flips Y. Needs no graphics card.
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
- `draw_system.c` — that the one entity a frame hides is not drawn and that every
  other one still is, in both tables and both layers, and that a red shaped cube
  reads red at the picture's centre. Skips without a graphics card.
- `shape_geometry.c` — that the CPU store answers for the three kinds and nothing
  else, that it holds each kind's own triangles with the cube's pointed straight
  at its constants, that every kind is a closed surface whose edges carry
  unit-length normals and whose cube has twelve folds and six face diagonals, and
  that every triangle of every kind is wound counter-clockwise seen from outside.
  Needs no graphics card.
- `pick.c` — the distance to a cube through the picture's centre, the ray that
  meets nothing, the nearer of two in either order, a cube found at the pixel its
  own centre projects to, a shape with no transform that is never answered, and —
  drawn — that a pixel the cube covers picks that cube, which is the check that
  the ray and the picture agree about which way is up. The arithmetic half needs
  no graphics card; the drawn half skips without one.
- `outline.c` — that a cube seen square on has four silhouette edges and one
  turned forty-five degrees six, that every corner of the quads is outside the
  face it hugs and none far from it, that twice as far away is twice as wide in
  metres and that the width is the pixels it was asked for, that every triangle
  faces the eye, that a capsule gives many edges and never more than the cap, and
  that a zeroed entity, a shape with no transform, a transform with no shape, an
  unknown kind and no store are each answered false with the answer left
  untouched. Needs no graphics card.
- `shape.c` — that kind is described, editable and named by its three names, and
  colour a described colour with no names, that the default row is a grey cube
  needing a transform, that an intent's new kind lands and re-points the mesh
  while an unknown one is put back and a colour clamped, that a run gives a
  shaped entity exactly one mesh and material and a second run adds nothing,
  that an entity without a shape is untouched, that an unknown kind gets
  nothing, that each kind gets its own geometry, that a removed shape's mesh
  and material are dropped while an imported mesh is kept, and that the
  capsule's and cylinder's normals are unit length, and that both are the right
  size and wound outward, and that the upload hands back both records — the
  shapes' lit white one and the outline's unlit one, on a shading of its own.
  The table and geometry half needs no graphics card; the upload half skips
  without one.
