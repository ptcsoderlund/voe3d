# tests

One plain C program per module, found by the build. The ones that measure what
reached the picture want a graphics card and say so; the rest want nothing.

- `projection.c` — reversed depth, the aspect ratio, and that nothing in the
  projection flips Y. Needs no graphics card.
- `depth_sort.c` — that the order visits the furthest away first, and that equal
  depths keep the order they came in. Its header says why a reversed sort is
  still a correct sort and what the sign looks like on screen. Needs no graphics
  card.
- `normal_matrix.c` — that a normal stays perpendicular to a non-uniformly scaled
  surface and that the world matrix's answer does not. Its header says which
  three cases would pass with the wrong matrix. Needs no graphics card.
- `panel.c` — the panel table, and that a panel is sorted among the see-through
  meshes rather than drawn in a pass of its own. Its header says why the ordering
  claim is measured with a draw that is deliberately refused, and why both halves
  of that measurement are needed. The table half needs no graphics card; the
  ordering half skips without one.
- `material.c` — that a material saying nothing about its UV rect reads its whole
  texture, and that one saying something keeps it. Its header says why this needs
  a graphics card to check a decision made on the CPU.
- `import.c` — a hand-built `.glb` in and entities out: the flattening, the
  shared picture, and one frame drawn from what it made. Its header says what
  every number in the file is for and that the mirrored model is what it is
  really looking for.
- `model_data.inc` — that file, as bytes.
- `draw_system.c` — that the one entity a frame hides is not drawn and that every
  other one still is, in both tables and both layers. Its header says why the
  draw count is the measurement, how the probe says which entity went, and what a
  wrong skip would look like. Skips without a graphics card.
- `shape.c` — that kind is described and read-only, that a run gives a shaped
  entity exactly one mesh and material and a second run adds nothing, that an
  entity without a shape is untouched, and that an unknown kind gets nothing. Its
  header says why the description is switched on regardless of the build. The
  table half needs no graphics card; the upload half skips without one.
