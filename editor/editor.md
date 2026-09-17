# editor

The program a person opens to author a scene. Today it opens a window on a top
bar — New, Open, Save, the project's name and a notice — above three columns:
`Scene` on the left, two scene views stacked in the middle, and `Inspector` on
the right. The left one lists the authored entities of the project it opens on
and a click on one selects it; each view draws the world from its own camera,
moved by a middle-button drag in it, lit by whichever light the world holds;
the right one lists what the selected entity is made of and lets a number in
it be dragged, which is what marks the project unsaved. Each column that is
not a scene view clips what is on it and scrolls it with the wheel or its
scrollbar, and the Inspector's field rows fold onto further lines when the
column is too narrow for them. `voe_editor [<folder>]` opens folder, or the
last project remembered when there is none, or an untitled cube and light when
there is neither — see `src/project.h` and `src/last_project.h`. New, Open and
Save — the buttons and Ctrl+N, Ctrl+O and Ctrl+S alike — go through
`src/session.h`, which refuses New, Open and closing the window once while
there are unsaved changes and goes ahead the second time. It can also be
started to draw one frame with no window at all, write it to a PNG file and
exit — `--capture <path>`, with `--size <W>x<H>` saying how big.

Open shows the editor's own file browser — an anchored panel over the dock,
below the bar — to choose a project's folder from; a folder marked "— project"
already holds one. Confirming it replaces the project on success, or leaves
the browser open with a notice on failure. Save on an untitled project shows
the same browser in SAVE mode instead, with a name box and a Make folder
button beside the confirm button, now "Save here": typing a name and either
pressing Enter in the box or clicking Make folder makes that folder and opens
it, and Save here writes `project.voe3d` and the scene into whichever folder
the browser is in, refusing one that is not empty. While the browser shows,
the top bar's clicks and the three shortcuts do nothing, and a scene view's
camera does not move; Escape or Cancel dismisses it without changing
anything.

It is a leaf and it stays one, exactly as `dev` is: it names whatever it needs
and nothing names it (ADR-0121). No engine folder gains anything for the
editor's sake — a gap in one of them is a card in that folder, never a
reach-around from here.

- `src/main.c` — reads the command line, opens whichever project the folder
  argument, the last one remembered or an untitled scene names before the
  device does, opens the window and the device through `voe_app_new` — or the
  device alone through `voe_app_new_headless` when a picture was asked for —
  makes the arena, the font, the interface context and the default tree,
  uploads the built-in shapes, and runs the loop until a close goes ahead or the
  picture is written. Ctrl+N, Ctrl+O and Ctrl+S and a window close all become a
  `voe_editor_session_do` call, and a refused close takes the window's own back.
  While the browser shows, the three shortcuts fire nothing and a
  middle-button drag moves no view's camera; Escape is read here too and
  handed to the interface as the browser's own Cancel. Backspace, Enter and
  `voe_platform_input_text` are read every frame, regardless of the browser,
  and handed to the interface as this frame's keyboard (task 14) — `ui` acts
  on them only for whichever field is focused. Its header says why the
  `while` is this file's while the parts in it are `app`'s (ADR-0135), why the
  one division that turns the mouse's pixels into the surface's millimetres is
  here and nowhere else (ADR-0141 point 4), what says how far a wheel notch
  moves anything, what a frame's passes are and in what order they run
  (ADR-0148), where the light every view is shown with comes from, what each
  argument does, when the last project is written and when it is not, and why
  a capture draws two frames before it writes.
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
- `src/session.h`, `src/session.c` — the project being worked on, its notice
  and the one armed command that makes closing the window, New and Open each
  refuse once while there are unsaved changes and go ahead the second time.
  Open, once allowed, shows the browser in OPEN mode; Save on an untitled
  project shows it in SAVE mode instead of writing anything. What a folder
  chosen in OPEN, a name made into a folder or entered with Enter (SAVE), or
  "Save here" does to the project — `voe_editor_session_browser_do`, on the
  browser's own Confirm and MAKE_FOLDER — is this file's too, the same shape
  as every other command. Its header says why only a `CLOSE` that goes ahead
  answers true, what NEW does to `scene` and to the old project, why an
  inspector edit disarms through a call of its own rather than through this
  file noticing it, and why a browser action that clears the notice and
  disarms only does either when something actually fired.
- `src/topbar.h`, `src/topbar.c` — the bar across the top of the root surface:
  New, Open, Save, the project's name and whether it is unsaved, then the
  session's notice. Its header says why it hands back which button fired
  rather than carrying a command out itself, and why its buttons are recorded
  and read afterwards exactly as the Scene panel's rows are.
- `src/browser.h`, `src/browser.c` — the editor's own file browser: a folder
  listing shown as an anchored panel over the dock, its own arena for the
  current folder and its rows, and what fired read back exactly as the top
  bar's buttons are. In SAVE mode it also draws a name row — a `ui` field
  holding a typed folder name, focused the frame it first shows, and a Make
  folder button — and keeps the typed text in its own buffer, written back
  every frame the way the inspector writes back a number box's value. Its
  header says why a listing failure changes nothing, why it keeps its folder
  (and its typed name) across showings for the whole run, and why entering a
  row, going up and making a folder are this file's own to carry out while
  what Confirm does to the project is session.h's.
- `src/dock.h`, `src/dock.c` — the tree, the walk, and `voe_editor_panel_draw`,
  which is where the panels' contents are. `voe_editor_dock_walk` takes the axis
  of whatever it is called inside, because interface.c now opens a column above
  it for the top bar. A root also carries this frame's keyboard beside its
  pointer (task 14), for `ui` to hand to whichever field is focused. Its
  header says why where a panel sits is a tree of data and not the order of
  the calls (ADR-0142), what the `fraction` is and that nothing writes one
  yet, why no function pointer lives in this folder, how a split hands its
  two children fixed millimetres and where a splitter would go, and why root
  zero is the window while a second root that is its own OS window is a card
  in three other folders before it is a line here.
- `src/interface.h`, `src/interface.c` — the screen-filling surface. Pixels per
  millimetre from the window's height over a 135 mm surface, the top bar laid
  above each root's dock tree in a column this file opens, the browser drawn
  over the tree when it shows, the records `ui` emitted submitted into the
  open frame, and one draw command per root. It hands each root's keyboard to
  `ui` beside its pointer and decides nothing about what is on a panel; its
  header says why the one question it does ask — what was clicked, the top
  bar's and the browser's included — has to be asked from in there, why
  nesting the tree under the bar's column means telling
  `voe_editor_dock_walk` which axis it is now inside, and why whether the
  browser was showing is captured once and used for every decision in a root's
  frame rather than read again after its own commands have run.
- `src/inspector.h`, `src/inspector.c` — what the selected entity is made of, and
  the controls that change it. It walks the world's component types and expands
  whatever came with a field description, so it names no component; an edit is a
  replace intent and never a write, and how many were submitted this frame is a
  count `main.c` reads to tell session.h the project changed. Its header says
  why the controls and every label's text have to outlive the call that drew
  them.
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
