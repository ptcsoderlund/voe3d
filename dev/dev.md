# dev

The one program a person runs to see what the engine can currently do. Today
that is a window holding a world: two models read out of `.glb` files, two cubes
placed by hand, and a camera that either orbits them or is flown with the
keyboard and the mouse. Not a menu of past states and not a test — it is looked
at, not asserted on.

- `src/main.c` — opens a window and a device, builds a world, reads both models
  into it, and runs the systems and the draw every frame until it closes. Its
  header is the list of things to look at in both camera modes, what each of them
  fails like, and the three things that live here only until the folder that owns
  them exists. A call site and nothing else; the key bindings are the only
  decision in it.
- `src/cubes.h` — the two placeholder cubes' geometry. Its header says why data
  may live at a call site and why there are twenty-four vertices.
- `src/cubes.c` — those vertices and indices.
- `src/texture.png` — the picture on the placeholder cubes, `#embed`ded at build
  time the way the shaders are, because `platform` has no file API to read one
  with. An "F" with four differently coloured corners: it is the shape that reads
  wrong under a flip and under a mirror, which is what makes "the right way up"
  something a person can check at a glance.
- `src/model.glb` — this repository's own test model, `#embed`ded for the same
  reason. One lettered cube with a second, half-sized one as its child, so that
  the import has a tree to flatten and a person can see whether the child landed
  where the composition of the two transforms says.
- `src/textured_primitives_human.glb` — a model a real exporter wrote: three
  primitives sharing one material, an albedo map and an ORM map. What it is for
  is the half of the reader that cannot be checked against a file this repository
  built itself.
