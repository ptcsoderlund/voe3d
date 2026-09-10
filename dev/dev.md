# dev

The one program a person runs to see what the engine can currently do. Today
that is a window holding a world: a model read out of a `.glb` file, two cubes
placed by hand, two see-through quads either side of them, a dozen sprites off a
sheet built in code, a lettered sign standing above them, a line of writing
locked to the camera, a panel of forty coloured rectangles and two lines of
writing standing among them, a small badge above them that nothing covers, a
plate and a row of ticks mapped onto the window itself, an interface with a
heading and two buttons on it that answer the mouse,
one sun going round it all, and a camera that either orbits them or is flown
with the keyboard and the mouse. Not a menu of past
states and not a test — it is looked at, not asserted on.

- `src/main.c` — opens a window and a device, builds a world, reads the model
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
  flying. Card 040 added a `draws` line — draw commands per frame, averaged and
  worsted over the same period as the timings, with the last frame's element
  record count beside it, which is ADR-0092's claim as two numbers rather than
  one. Its header says why the worst column is the point of making it a metric,
  what the first frame shows instead, and why the count includes the readout's
  own draw. Card 030 added the element exhibit, which is two calls and a printed
  measurement here and nothing else — see `src/elements.h`; card 031 put writing
  on it. Card 032 made the exhibit an entity with a panel component standing in
  the world, added a badge in the overlay beside it, and left a third surface
  mapped onto the window: its header now lists the three and says which one a
  resize moves, and which failure "the exhibit visible through a cube" is.
- `src/elements.h` — the two panels' content: the exhibit's forty rectangles and
  forty letters, and the badge's five, submitted every frame and each drawn by
  one command. Its header says why nothing in there draws, that both surfaces are
  objects in metres whose millimetres are real millimetres, why one stands in the
  world and the other is in the overlay, and why the count is two numbers now
  that one of them depends on what the writing says.
- `src/elements.c` — those eighty elements, in the order they are painted: the
  backing panel, the bar that is twice as wide as its clip rectangle, the six
  bars at six alphas, the thirty-two squares of thirty-two colours, and two lines
  of writing — one large and one small enough to show what the sheet cannot hold.
  Then the badge's five. Its header says what each group is there to show, what a
  wrong premultiply would look like, where the one conversion from a font's ems
  and +y up to the surface's millimetres and y down is written, and why the badge
  is deliberately asymmetric.
- `src/interface.h` — the first real interface: a semitransparent panel with a
  heading and two buttons on it, laid out by `ui` and answering the mouse. Its
  header says why the draw count is the claim and why the interface is its own
  surface, why it is mapped onto the window rather than standing in the world,
  and why its panel is pushed down the page with a spacer until card 041.
- `src/interface.c` — the panel's contents and the one division that turns the
  mouse's pixels into the surface's millimetres. Its header says why one button
  counts and the other does nothing, and why the count lives here rather than
  in `ui`.
- `src/surface.h` — the screen-filling surface: a plate, a square and a row of
  ticks mapped straight onto the window, and `VOE_DEV_UI_SCALE`, which is the
  only calibration this engine has and is changed here. Its header says how it
  differs from a panel, why an authored millimetre means something different on
  it, what a resize does to it and why content falling off a narrow window's
  right edge is correct.
- `src/surface.c` — those rectangles and the one multiplication that turns the
  window's height into pixels per millimetre. Its header says why the ticks are
  meant to run off the edge, why the square is square, why the plate sits against
  the right edge rather than in the readout's corner, and why its x is the one
  thing here anchored rather than authored.
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
- `src/shrink.h` — a decoded picture halved until neither side is longer than
  1920, `dev`'s own and not a rule about textures. Its header says why this is
  not mipmapping and cannot be, what it buys in numbers, and what it does not
  fix.
- `src/shrink.c` — the halving. Its header says why the average is taken in
  linear light and what the number is if it is not, why alpha is left out of
  that, and why writing over the picture being read is safe.
- `src/logo.png` — the picture on the turning cube: the VOE3D wordmark,
  `#embed`ded at build time the way the shaders are, because `platform` has no
  file API to read one with. It is wide, and so is that cube's front face, which
  is why it goes on this one and not the other.
- `src/app icon light.png` — the picture on the still cube, square like the cube
  and embedded the same way. Writing reads wrong under a flip and under a
  mirror, and the yellow badge is in one corner only, which is what makes "the
  right way up" something a person can check at a glance.
- `src/textured_primitives_human.glb` — a model a real exporter wrote: three
  primitives sharing one material, an albedo map and an ORM map. What it is for
  is the half of the reader that cannot be checked against a file this repository
  built itself, and now also the only surface in the scene whose roughness and
  metalness come out of a picture.
