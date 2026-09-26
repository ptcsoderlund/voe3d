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
  frames blind, a red shape reads red, a gizmo shows through its cube and a camera marker is one
  draw more. Skips without a graphics card.
- `no_light.c` — that a world with no light frames unshaded and not blind, and one light frames as
  itself. Needs no graphics card.
- `shape_geometry.c` — that the CPU store answers for the three kinds and nothing else, holds each
  kind's own triangles, and that every kind is a closed surface wound counter-clockwise seen from
  outside. Needs no graphics card.
- `pick.c` — that the pick ray meets the nearest cube or camera box at the right distance and
  nothing where nothing is, and that a drawn pixel the cube covers picks it. The arithmetic half
  needs no graphics card; the drawn half skips without one.
- `outline.c` — the silhouette edge counts of a cube square on and turned, the quads' corners, width
  and winding, and — drawn — that a hidden cube's outline shows through. The arithmetic half needs
  no graphics card; the drawn half skips without one.
- `camera_marker.c` — the marker's twenty edges' worth of geometry, a camera scaled to nothing
  building nothing and not hit, and a ray meeting the box square on, turned and not at all. Needs
  no graphics card.
- `collider_marker.c` — a box's twelve edges, a sphere's and a capsule's counts, a box twice the
  size twice as far out, quads about the eye 100 km out, and the collider that fits each shape.
  Needs no graphics card.
- `gizmo.c` — a ray across each arrow and through each square, the space that meets nothing, the
  shaft doubling with the distance, the two grabs and the one refusal, and the two meshes' counts,
  their winding towards the eye and the handle marking moves across. Needs no graphics card.
- `far.c` — that everything moved 100 km out picks the same cube at the same distance, grabs a
  millimetre as a millimetre, and frames about the camera's double position. Needs no graphics
  card.
- `shape.c` — the shape table's description, default row, intents and runs, each kind's own
  geometry, and the upload's two material records. The table and geometry half needs no graphics
  card; the upload half skips without one.
