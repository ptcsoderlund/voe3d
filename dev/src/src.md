# src

`dev`'s implementation. There is no public header: `main.c` is the program,
and every other code file is a `.h` saying what it owns and why beside the `.c`
that carries it out. The pictures and the model it embeds sit here too.

- `main.c` — opens a window and a device, builds a world, reads the model
  into it, and runs the systems and the draw every frame until it closes. The
  window and the device come from `voe_app_new`, the clock and the poll from
  `voe_app_frame_open`, and the draw is opened and closed by `voe_app_draw_open`
  and `voe_app_draw_close` — the `while` itself, the order the systems run in and
  everything submitted between two of them stay here. Two passes a frame: the
  monitor's target first, then the window. It owns the frame and puts
  a block of timings on the screen every couple of seconds. Its header is the list of things to look at in both camera
  modes, what each of them fails like — the lighting's and the blending's own
  failures included — and the three things that live here only until the folder
  that owns them exists. On the readout: what each of the four numbers brackets,
  in what order a frame's phases run and why, why it is rebuilt every frame and
  never cached, what a readout that stops changing means, what to look for on the
  `mouse` line — the pointer's position and the three buttons, live — at each
  corner, off the window and while flying, and, on the `draws` line, which is
  draw commands per frame averaged and worsted over the same period as the
  timings with the last frame's element record count beside it, ADR-0092's claim
  as two numbers rather than one: why the worst column is the point of making it
  a metric, what the first frame shows instead, and why the count includes the
  readout's own draw. On the two see-through quads either side of the cubes: what
  their overlap is showing and which failure a wrong sort looks like. On the two
  strings: why the sign is two entities sharing one mesh, what the five things a
  text material has to say are, why the line locked to the camera is an ordinary
  transform in the world rather than anything screen-space, what a letter walked
  up to should and should not look like, why the line sits on a dark panel and
  what that panel does and does not prove, why it is placed between two systems
  rather than with the other intents, and why neither string holds a character
  outside Latin-1. A call site and nothing else; the key bindings and asking for
  mailbox at startup are the only decisions in it, P among them. The sprites are a call and a per-frame call here
  and nothing else — see `sprites.h` — and the element exhibit is two calls
  and a printed measurement, with writing on it — see `elements.h`. That
  exhibit is an entity with a panel component standing in the world, a badge sits
  in the overlay beside it and a third surface is mapped onto the window: the
  header lists the three and says which one a resize moves, and which failure
  "the exhibit visible through a cube" is.
- `monitor.h` — the second camera's picture: a target, a camera that is
  not an entity, and the screen standing in the world that wears the target's
  texture. Its header says why the camera may not be an entity, why the pass
  that fills the target hides the screen and what is seen when it does not,
  and why the screen is one-sided and unlit.
- `monitor.c` — that target, that camera, the screen's one face and the
  frame the monitor's pass is drawn with. Its header says where the four
  vertices came from and why (0, 0) is its top-left.
- `elements.h` — the two panels' content: the exhibit's forty rectangles and
  forty letters, and the badge's five, submitted every frame and each drawn by
  one command. Its header says why nothing in there draws, that both surfaces are
  objects in metres whose millimetres are real millimetres, why one stands in the
  world and the other is in the overlay, and why the count is two numbers now
  that one of them depends on what the writing says.
- `elements.c` — those eighty elements, in the order they are painted: the
  backing panel, the bar that is twice as wide as its clip rectangle, the six
  bars at six alphas, the thirty-two squares of thirty-two colours, and two lines
  of writing — one large and one small enough to show what the sheet cannot hold.
  Then the badge's five. Its header says what each group is there to show, what a
  wrong premultiply would look like, where the one conversion from a font's ems
  and +y up to the surface's millimetres and y down is written, and why the badge
  is deliberately asymmetric.
- `interface.h` — the first real interface: a semitransparent panel with a
  heading, two buttons and three number boxes on it, laid out by `ui` and
  answering the mouse. Its header says why the draw count is the claim and holds
  whatever the number boxes add to it, why the interface is its own surface, why it is mapped onto the window rather than standing in the world,
  and why its panel is pushed down the page to clear the readout rather than
  anchored where it wants to be.
- `interface.c` — the panel's contents and the one division that turns the
  mouse's pixels into the surface's millimetres. Its header says why one button
  counts and the other does nothing, why the count and the three dragged values
  live here rather than in `ui`, why each number box changes the panel it stands
  on, and why the clamping and the rounding are this file's.
- `surface.h` — the screen-filling surface: a plate, a square and a row of
  ticks mapped straight onto the window, and `VOE_DEV_UI_SCALE`, which is the
  only calibration this engine has and is changed here. Its header says how it
  differs from a panel, why an authored millimetre means something different on
  it, what a resize does to it and why content falling off a narrow window's
  right edge is correct.
- `surface.c` — those rectangles and the one multiplication that turns the
  window's height into pixels per millimetre. Its header says why the ticks are
  meant to run off the edge, why the square is square, why the plate sits against
  the right edge rather than in the readout's corner, and why its x is the one
  thing here anchored rather than authored.
- `sprites.h` — the sprite exhibit: the sheet, the twelve entities and the
  two rotations that turn towards the camera. Its header says why the
  billboarding is here and not in the engine.
- `sprites.c` — the sheet's pixels, where each sprite stands and the two
  rotations. Its header says why the disc's edge is soft and its surroundings
  empty, what a halo round one would mean, and what each group of sprites is
  there to show — the two layers, the two alpha modes, the sort, and the one
  extra term that is the whole difference between cylindrical and spherical.
- `cubes.h` — the two placeholder cubes' geometry. Its header says why data
  may live at a call site and why there are twenty-four vertices.
- `cubes.c` — those vertices and indices.
- `quad.h` — the see-through quads' geometry. Its header says why it is two
  faces in the same place, and why that is a double-sided mesh rather than a
  double-sided material.
- `quad.c` — those vertices and indices.
- `shrink.h` — a decoded picture halved until neither side is longer than
  1920, `dev`'s own and not a rule about textures. Its header says why this is
  not mipmapping and cannot be, what it buys in numbers, and what it does not
  fix.
- `shrink.c` — the halving. Its header says why the average is taken in
  linear light and what the number is if it is not, why alpha is left out of
  that, and why writing over the picture being read is safe.
- `logo.png` — the picture on the turning cube: the VOE3D wordmark,
  `#embed`ded at build time the way the shaders are, because `platform` has no
  file API to read one with. It is wide, and so is that cube's front face, which
  is why it goes on this one and not the other.
- `app icon light.png` — the picture on the still cube, square like the cube
  and embedded the same way. Writing reads wrong under a flip and under a
  mirror, and the yellow badge is in one corner only, which is what makes "the
  right way up" something a person can check at a glance.
- `textured_primitives_human.glb` — a model a real exporter wrote: three
  primitives sharing one material, an albedo map and an ORM map. What it is for
  is the half of the reader that cannot be checked against a file this repository
  built itself, and now also the only surface in the scene whose roughness and
  metalness come out of a picture.
