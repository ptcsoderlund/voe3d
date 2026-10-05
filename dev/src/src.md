# src

`dev`'s implementation. There is no public header: `main.c` is the program,
and every other code file is a `.h` saying what it owns and why beside the `.c`
that carries it out. The pictures and the model it embeds sit here too.

- `main.c` — the loop: every frame it takes the keys, moves the eye and the sun, runs the systems
  in order, draws the world twice and takes the frame breakdown, until the window closes. Its header gives that order, and what
  is wrong if the picture looks wrong and what there is to try stand above `main()`.
- `startup.h` — the program's state: one member per thing the loop reads, written before the first
  frame and by the loop, and read nowhere else.
- `startup.c` — everything built before that frame, in the order it has to happen in: the arenas,
  the window and the device, the world and its components, every exhibit, the model and the monitor,
  with the numbers they are made with.
- `monitor.h` — the second camera's picture: a target, a pose and a lens that are not an entity,
  and the screen standing in the world that wears the target's texture.
- `monitor.c` — that target, that pose and lens, the screen's one face and the frame the monitor's
  pass is drawn with, its view from `voe_3d_view`. Its header says where the four vertices came from
  and why (0, 0) is its top-left.
- `elements.h` — the two panels' content: the exhibit's forty rectangles and forty letters, and the
  badge's five, submitted every frame and each drawn by one command.
- `elements.c` — those eighty elements in the order they are painted — the backing panel, the bars,
  the thirty-two squares and two lines of writing — and then the badge's five.
- `interface.h` — the first real interface: a semitransparent panel with a heading, three buttons
  and three number boxes on it, laid out by `ui` and answering the mouse; Frame toggles the
  breakdown.
- `interface.c` — the panel's contents, the breakdown drawn when shown, and the one division that
  turns the mouse's pixels into the surface's millimetres.
- `breakdown.h` — the frame breakdown: each pass's GPU milliseconds by render's name and the total,
  copied four times a second, and its cost in the interface's budget.
- `breakdown.c` — that copy and the panel anchored at the interface's top right, a row per pass.
- `surface.h` — the screen-filling surface: a plate, a square and a row of ticks mapped straight
  onto the window, and `VOE_DEV_UI_SCALE`, the only calibration this engine has.
- `surface.c` — those rectangles and the one multiplication that turns the window's height into
  pixels per millimetre.
- `sprites.h` — the sprite exhibit: the sheet, the twelve entities and the two rotations that turn
  towards the camera, from this frame's flight. Its header says why the billboarding is here and not
  in the engine.
- `sprites.c` — the sheet's pixels, where each sprite stands and the two rotations that turn them
  towards the camera.
- `cubes.h` — the two placeholder cubes as an exhibit: their geometry and the call that builds
  them. Its header says why data may live at a call site and why there are twenty-four vertices.
- `cubes.c` — those vertices and indices, the two pictures they wear, and the two entities, with
  what is wrong if they look wrong.
- `quad.h` — the see-through quads as an exhibit: their geometry, and the calls that build a quad,
  a panel and all five. Its header says why it is two faces in the same place.
- `quad.c` — those vertices and indices, and the two quads in the world and three in the overlay,
  with what each is for and what is wrong if it looks wrong.
- `text.h` — the writing exhibit: the font, the sign in the world, the heads-up line on its panel
  and the readout's entity.
- `text.c` — those strings and materials, the two placements text has, and what is wrong if the
  writing looks wrong.
- `facing.h` — the transforms that turn the heads-up line, its panel and the readout to the camera,
  from the eye's pose this frame, and who calls them each frame.
- `facing.c` — the eye's frame read from its pose and the three placements built from it.
- `motion.h` — how the eye and the sun move each frame: the flight the keys fly, the orbit at a
  second, the pose a flight is seen from and `voe_dev_sun_pose`, the sun's transform turned along
  its lap. Its header says why the cube's spin is not here.
- `motion.c` — those answers and the numbers behind them: the bindings, speeds and pitch limit and
  what is wrong if flying feels wrong, the orbit's radius, height and lap, and the sun's lap, height
  and where its marker stands.
- `readout.h` — what the loop measures: the samples the frame is timed with, the reporting interval,
  the readout's em, and the three calls that print the legend, lay the readout out and print a block.
- `readout.c` — the block's wording and the readout's seven lines, with why a line shows the last
  period's number for the one frame a new period has none of its own.
- `model.h` — the model exhibit: the embedded `.glb` and the call that reads it in and moves it.
- `model.c` — that import, the intents that move it aside, and what is wrong if it looks wrong.
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
- `textured_primitives_human.glb` — a model a real exporter wrote: three primitives sharing one
  material, an albedo map and an ORM map, the half of the reader no file this repository built can
  check.
