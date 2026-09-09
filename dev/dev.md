# dev

The one program a person runs to see what the engine can currently do. Today
that is a window holding a world: two models read out of `.glb` files, two cubes
placed by hand, two see-through quads either side of them, a dozen sprites off a
sheet built in code, a lettered sign standing above them, a line of writing
locked to the camera, one sun going round it all, and a camera that either
orbits them or is flown with the keyboard and the mouse. Not a menu of past
states and not a test — it is looked at, not asserted on.

- `src/main.c` — opens a window and a device, builds a world, reads both models
  into it, and runs the systems and the draw every frame until it closes. Its
  header is the list of things to look at in both camera modes, what each of them
  fails like — the lighting's and the blending's own failures included — and the
  three things that live here only until the folder that owns them exists. Card
  020 gave it a real clock and a block of timings every couple of seconds, and
  card 021a put two see-through quads either side of the cubes; its header says
  what each of the four numbers brackets, what the overlap between the quads is
  showing, and which failure a wrong sort looks like. Card 021b added the two
  strings; its header says why the sign is two entities sharing one mesh, what
  the five things a text material has to say are, why the line locked to the
  camera is an ordinary transform in the world rather than anything
  screen-space, what a letter walked up to should and should not look like, why
  the line sits on a dark panel and what that panel does and does not prove, why it is placed between two systems rather than with the other
  intents, and why neither string holds a character outside Latin-1. A call
  site and nothing else; the key bindings are the only decision in it, P among
  them. Card 022 added the sprites, which are a call and a per-frame call and
  nothing else here — see `src/sprites.h`. Card 028 put the timing numbers on
  the screen and made the loop own the frame: its header says in what order a
  frame's phases run and why, why the readout is rebuilt every frame and never
  cached, and what a readout that stops changing means. Card 029 added a
  `mouse` line to it — the pointer's position and the three buttons, live — and
  its header says what to look for at each corner, off the window, and while
  flying.
- `src/sprites.h` — the sprite exhibit: the sheet, the twelve entities and the
  two rotations that turn towards the camera. Its header says why the
  billboarding is here and not in the engine.
- `src/sprites.c` — the sheet's pixels, where each sprite stands and the two
  rotations. Its header says why the disc's edge is soft and its surroundings
  empty, what a halo round one would mean, and what each group of sprites is
  there to show — the two layers, the two alpha modes, the sort, and the one
  extra term that is the whole difference between cylindrical and spherical.
- `src/cubes.h` — the two placeholder cubes' geometry. Its header says why data
  may live at a call site and why there are twenty-four vertices.
- `src/cubes.c` — those vertices and indices.
- `src/quad.h` — the see-through quads' geometry. Its header says why it is two
  faces in the same place, and why that is a double-sided mesh rather than a
  double-sided material.
- `src/quad.c` — those vertices and indices.
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
  built itself, and now also the only surface in the scene whose roughness and
  metalness come out of a picture.
