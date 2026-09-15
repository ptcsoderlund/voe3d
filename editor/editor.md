# editor

The program a person opens to author a scene. Today it opens a window on three
columns — `Scene` on the left, two scene views stacked in the middle, and
`Inspector` on the right. The left one lists the authored entities of a scene
built in code and a click on one selects it; each view draws the scene's cubes
from its own camera, moved by a middle-button drag in it; the right one lists what
the selected entity is made of and lets a number in it be dragged. Each column
that is not a scene view clips what is on it and scrolls it with the wheel or its
scrollbar, and the Inspector's field rows fold onto further lines when the column
is too narrow for them. It can also be started to draw one frame with no window
at all, write it to a PNG file and exit — `--capture <path>`, with `--size
<W>x<H>` saying how big.

It is a leaf and it stays one, exactly as `dev` is: it names whatever it needs
and nothing names it (ADR-0121). No engine folder gains anything for the
editor's sake — a gap in one of them is a card in that folder, never a
reach-around from here.

- `src/main.c` — reads the command line, opens the window and the device through
  `voe_app_new` — or the device alone through `voe_app_new_headless` when a
  picture was asked for — makes the arena, the world, the font, the interface
  context and the default tree, registers the components, builds the scene, and
  runs the loop until the window closes or the picture is written. Its header
  says why the `while` is this file's while the parts in it are `app`'s
  (ADR-0135), why the one division that turns the mouse's pixels into the
  surface's millimetres is here and nowhere else (ADR-0141 point 4), what says
  how far a wheel notch moves anything, what a frame's passes are and in what
  order they run (ADR-0148), what each argument does and why a capture draws two
  frames before it writes.
- `src/dock.h`, `src/dock.c` — the tree, the walk, and `voe_editor_panel_draw`,
  which is where the panels' contents are. Its header says why where a panel sits
  is a tree of data and not the order of the calls (ADR-0142), what the `fraction`
  is and that nothing writes one yet, why no function pointer lives in this folder,
  how a split hands its two children fixed millimetres and where a splitter would
  go, and why root zero is the window while a second root that is its own OS window
  is a card in three other folders before it is a line here.
- `src/interface.h`, `src/interface.c` — the screen-filling surface. Pixels per
  millimetre from the window's height over a 135 mm surface, the records `ui`
  emitted submitted into the open frame, and one draw command per root. It
  decides nothing about what is on a panel, and its header says why the one
  question it does ask — what was clicked — has to be asked from in there.
- `src/inspector.h`, `src/inspector.c` — what the selected entity is made of, and
  the controls that change it. It walks the world's component types and expands
  whatever came with a field description, so it names no component; an edit is a
  replace intent and never a write. Its header says why the controls and every
  label's text have to outlive the call that drew them.
- `src/view.h`, `src/view.c` — a scene view: its camera, its target, the
  middle-button drag and the editor's sun. Its header says why the camera is not
  an entity, why the picture's size lags the layout by a frame, and why a view
  whose leaf is not in the tree is not drawn.
- `src/cube.h`, `src/cube.c` — the cube the scene's two cubes are drawn with,
  copied from `dev` as placeholder data.
- `src/scene.h`, `src/scene.c` — the four entities the editor opens on, built in
  code until there is a loader, and the selection. Its header says why three of
  the four are authored and the fourth deliberately is not, why the selection is
  the editor's and not the dock tree's, and why the rows the Scene panel drew
  have to outlive the call that drew them. It also carries what the Inspector
  drew this frame, for the same reason and in inspector.h's own struct.
