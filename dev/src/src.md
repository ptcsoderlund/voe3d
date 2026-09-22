# src

`dev`'s implementation. There is no public header: `main.c` is the program,
and every other code file is a `.h` saying what it owns and why beside the `.c`
that carries it out. The pictures and the model it embeds sit here too.

- `main.c` — opens a window and a device, builds a world, reads the model into it, and runs the
  systems and the draw every frame until it closes. Its header gives the loop's order; each exhibit's
  paragraph, what fails and what to try stand above the function that builds or drives it.
- `monitor.h` — the second camera's picture: a target, a camera that is not an entity, and the
  screen standing in the world that wears the target's texture.
- `monitor.c` — that target, that camera, the screen's one face and the
  frame the monitor's pass is drawn with. Its header says where the four
  vertices came from and why (0, 0) is its top-left.
- `elements.h` — the two panels' content: the exhibit's forty rectangles and forty letters, and the
  badge's five, submitted every frame and each drawn by one command.
- `elements.c` — those eighty elements in the order they are painted — the backing panel, the bars,
  the thirty-two squares and two lines of writing — and then the badge's five.
- `interface.h` — the first real interface: a semitransparent panel with a heading, two buttons and
  three number boxes on it, laid out by `ui` and answering the mouse.
- `interface.c` — the panel's contents and the one division that turns the mouse's pixels into the
  surface's millimetres.
- `surface.h` — the screen-filling surface: a plate, a square and a row of ticks mapped straight
  onto the window, and `VOE_DEV_UI_SCALE`, the only calibration this engine has.
- `surface.c` — those rectangles and the one multiplication that turns the window's height into
  pixels per millimetre.
- `sprites.h` — the sprite exhibit: the sheet, the twelve entities and the
  two rotations that turn towards the camera. Its header says why the
  billboarding is here and not in the engine.
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
  and who calls them each frame.
- `facing.c` — the camera's frame and the three placements built from it.
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
