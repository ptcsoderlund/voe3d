# editor

The program a person opens to author a scene. Today it opens a window on three
columns — `Scene` on the left, two scene views stacked in the middle, and
`Inspector` on the right. The left one lists the authored entities of the
project it opens on and a click on one selects it; each view draws the world
from its own camera, moved by a middle-button drag in it, lit by whichever
light the world holds; the right one lists what the selected entity is made of
and lets a number in it be dragged. Each column that is not a scene view clips
what is on it and scrolls it with the wheel or its scrollbar, and the
Inspector's field rows fold onto further lines when the column is too narrow
for them. `voe_editor [<folder>]` opens folder, or the last project remembered
when there is none, or an untitled cube and light when there is neither — see
`src/project.h` and `src/last_project.h`. It can also be started to draw one
frame with no window at all, write it to a PNG file and exit — `--capture
<path>`, with `--size <W>x<H>` saying how big.

It is a leaf and it stays one, exactly as `dev` is: it names whatever it needs
and nothing names it (ADR-0121). No engine folder gains anything for the
editor's sake — a gap in one of them is a card in that folder, never a
reach-around from here.

- `src/main.c` — reads the command line, opens whichever project the folder
  argument, the last one remembered or an untitled scene names before the
  device does, opens the window and the device through `voe_app_new` — or the
  device alone through `voe_app_new_headless` when a picture was asked for —
  makes the arena, the font, the interface context and the default tree,
  uploads the built-in shapes, and runs the loop until the window closes or the
  picture is written. Its header says why the `while` is this file's while the
  parts in it are `app`'s (ADR-0135), why the one division that turns the
  mouse's pixels into the surface's millimetres is here and nowhere else
  (ADR-0141 point 4), what says how far a wheel notch moves anything, what a
  frame's passes are and in what order they run (ADR-0148), where the light
  every view is shown with comes from, what each argument does, when the last
  project is written and when it is not, and why a capture draws two frames
  before it writes.
- `src/project.h`, `src/project.c` — the project being worked on: its own
  arena, the world in it (every component type a project may hold registered
  the same way whether the scene is untitled or read off disk), the kept
  sections it was read with, its absolute folder and whether it has unsaved
  changes (ADR-0164). Its header says why every failure to open one destroys
  the arena it was building, what an untitled project's Save refuses, and why
  the scene file this editor writes is always named `main.scene`.
- `src/last_project.h`, `src/last_project.c` — the one remembered folder at
  `<settings>/voe3d/last_project`, read as its one line and written by making
  the two folders above it as needed. Its header says why a first start is not
  a failure worth reporting.
- `src/notice.h`, `src/notice.c` — one line long enough to explain why a
  project failed to open or save, built either from a caller's own words or
  from base/report.h's first kept error. Its header says what a caller has to
  do before asking for the latter.
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
- `src/view.h`, `src/view.c` — a scene view: its camera, its target and the
  middle-button drag. Its header says why the camera is not an entity, why the
  light is the world's and not a view's own, why the picture's size lags the
  layout by a frame, and why a view whose leaf is not in the tree is not drawn.
- `src/scene.h`, `src/scene.c` — the current project's world, the selection in
  it, and the rows the Scene panel drew. Its header says why building a
  project's entities is not this file's job, why the selection is the editor's
  and not the dock tree's, and why the rows the Scene panel drew have to
  outlive the call that drew them. It also carries what the Inspector drew
  this frame, for the same reason and in inspector.h's own struct.
