# tests

`3d`'s own tests: plain C programs with an ordinary `main()`, zero for pass, found by the build and
registered nowhere. A reader is here to find which file already makes a claim before making it
again, or to find where a claim that has started failing is written down.

- `projection.c` — reversed depth, the aspect ratio, that nothing in the
  projection flips Y, and the view of a pose and a lens, a flattened pose seeing
  nothing. Needs no graphics card.
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
- `draw_system.c` — that a camera scaled to nothing frames blind, that the entity a frame hides is not drawn and every other
  one is, in both tables and both layers, that a red shaped cube reads red at the
  picture's centre, and that a gizmo in a cube is read where the cube alone would
  be. Skips without a graphics card.
- `shape_geometry.c` — that the CPU store answers for the three kinds and nothing else, holds each
  kind's own triangles, and that every kind is a closed surface wound counter-clockwise seen from
  outside. Needs no graphics card.
- `pick.c` — the distance to a cube through the picture's centre, the ray that meets nothing, the
  nearer of two in either order, and — drawn — that a pixel the cube covers picks that cube. The
  arithmetic half needs no graphics card; the drawn half skips without one.
- `outline.c` — the silhouette edge counts of a cube square on and turned, the quads' corners, width
  and winding, and — drawn — that a hidden cube's outline shows through. The arithmetic half needs
  no graphics card; the drawn half skips without one.
- `gizmo.c` — a ray across each arrow and through each square, the space that meets nothing, the
  shaft doubling with the distance, the two grabs and the one refusal, and the two meshes' counts,
  their winding towards the eye and the handle marking moves across. Needs no graphics card.
- `shape.c` — the shape table's description, default row, intents and runs, each kind's own
  geometry, and the upload's two material records. The table and geometry half needs no graphics
  card; the upload half skips without one.
