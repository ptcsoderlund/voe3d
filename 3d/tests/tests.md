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
- `shape.c` — that kind is described and read-only and colour a described
  colour, that the default row is a grey cube needing a transform, that an
  intent lands with kind put back and colour clamped, that a run gives a shaped
  entity exactly one mesh and material and a second run adds nothing, that an
  entity without a shape is untouched, that an unknown kind gets nothing, that
  each kind gets its own geometry, that a removed shape's mesh and material are
  dropped while an imported mesh is kept, and that the capsule's and cylinder's
  normals are unit length, and that both are the right size and wound outward. The table and geometry half needs
  no graphics card; the upload half skips without one.
