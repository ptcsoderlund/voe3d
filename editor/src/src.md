# src

`editor`'s implementation. There is no public header: `main.c` is the program,
and every other file is a `.h` saying what it owns and why beside the `.c` that
carries it out.

- `main.c` — reads the command line, opens whichever project the folder
  argument, the last one remembered or an untitled scene names before the
  device does, opens the window and the device through `voe_app_new` — or the
  device alone through `voe_app_new_headless` when a picture was asked for —
  makes the arena, both fonts, the themes, the interface context in the
  chosen theme and the default tree, sets a good save of the chosen theme's
  file on the context and puts a refused one's report in the session's notice,
  uploads the built-in shapes, and runs the loop until a close goes ahead or the
  picture is written. Ctrl+N, Ctrl+O and Ctrl+S and a window close all become a
  `voe_editor_session_do` call, and a refused close takes the window's own back.
  While the browser shows, the three shortcuts fire nothing and a
  middle-button drag moves no view's camera; Escape is read here too and
  handed to the interface as the browser's own Cancel, and hides Preferences
  while the browser does not show. Backspace, Enter and
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
- `project.h` — the project being worked on: its own arena, the world in it
  (every component type a project may hold registered the same way whether the
  scene is untitled or read off disk), the kept sections it was read with, its
  absolute folder and whether it has unsaved changes (ADR-0164). Its header
  says why every failure to open one destroys the arena it was building, what
  an untitled project's Save refuses, and why the scene file this editor
  writes is always named `main.scene`.
- `project.c` — opening, making and saving a project, the one `world_new` every
  world is built by, and the untitled scene's cube and light, which are this
  file's decision and not `scene.c`'s.
- `last_project.h` — the one remembered folder at
  `<settings>/voe3d/last_project`. Its header says why a first start is not a
  failure worth reporting.
- `last_project.c` — reading that file as its one line and writing it by making
  the two folders above it as needed.
- `themes.h` — Near black and Near white, the two themes with no file, then
  one per `*.theme` file in `<settings>/voe3d/themes/`, each file's in an arena
  of its own, and the one chosen, remembered in `<settings>/voe3d/theme` as an
  empty line, `near_white` or a file's name, whose file is
  read again once a second and its palette replaced when a save reads; the
  built-ins are in Pixel Operator, and a font override remembered in
  `<settings>/voe3d/font` is applied to a copy of the chosen palette, the one
  the interface is set with. Its
  header says why each theme's arena is its own, what a file that will not
  read leaves behind, when loading answers false, why the live check compares
  the bytes and not a timestamp (ADR-0172), and what a refused save leaves
  behind, and why the override leaves the entries alone.
- `themes.c` — the folder listed and made, each file read and derived with the
  face it names, the two remembered files read and written as one line each,
  the palette in force refreshed, and the
  chosen file's once-a-second re-read.
- `notice.h` — one line long enough to explain why a project failed to open or
  save. Its header says what a caller has to do before asking for one built
  from base/report.h's first kept error.
- `notice.c` — a notice's text cleared, set from a format, or built from
  base/report.h's first kept error.
- `session.h` — the project being worked on, its notice and the one armed
  command that makes closing the window, New and Open each refuse once while
  there are unsaved changes and go ahead the second time. Open, once allowed,
  shows the browser in OPEN mode; Save on an untitled project shows it in SAVE
  mode instead of writing anything; what a browser action does to the project
  is `voe_editor_session_browser_do`. Its header says why only a `CLOSE` that
  goes ahead answers true, what NEW does to `scene` and to the old project, why
  an inspector edit disarms through a call of its own, and why a browser action
  clears the notice and disarms only when something actually fired.
- `session.c` — the refuse-once rule, the four commands, and what a browser
  action does to the session.
- `topbar.h` — the bar across the top of the root surface: New, Open, Save,
  Preferences, the project's name and whether it is unsaved, then the
  session's notice. Its header says why it hands back which button fired rather than carrying a
  command out itself, and why its buttons are recorded and read afterwards
  exactly as the Scene panel's rows are.
- `topbar.c` — the bar's one frame of `ui` calls, a panel holding one row, and
  the read of its four buttons afterwards.
- `preferences.h` — Preferences: one row per theme with its name and a Choose
  button, the one in force marked, and Close, as an anchored panel over the
  dock. Its header says why each row is drawn in its own theme, why Choose is
  carried out elsewhere, and how many themes it lists.
- `preferences.c` — the panel's one frame of `ui` calls and the read of its
  buttons afterwards.
- `browser.h` — the editor's own file browser: a folder listing shown as an
  anchored panel over the dock, its own arena for the current folder and its
  rows, and in SAVE mode a name row with a focused `ui` field and a Make folder
  button. Its header says why a listing failure changes nothing, why it keeps
  its folder and typed name across showings, and why entering a row, going up
  and making a folder are its own to carry out while what Confirm does to the
  project is session.h's.
- `browser.c` — the browser's listing, its one frame of `ui` calls, and the read
  of its buttons and rows afterwards.
- `dock.h` — the tree, the walk, and `voe_editor_panel_draw`, which is where the
  panels' contents are; a root carries this frame's keyboard beside its
  pointer. Its header says why where a panel sits is a tree of data and not the
  order of the calls (ADR-0142), what the `fraction` is, why no function
  pointer lives in this folder, and why root zero is the window.
- `dock.c` — the walk from a tree of nodes to one frame of `ui` calls, giving
  every child a fixed size in millimetres, a one-millimetre gap at each seam
  where a splitter would go, and the row the tree is wrapped in.
- `interface.h` — the screen-filling surface, made in the theme it is handed:
  pixels per millimetre from the window's height over a 135 mm surface, the top
  bar above each root's dock tree, the browser or Preferences over it when one
  shows, and one draw command per root. Its header says why the one question it asks — what was clicked — has to be
  asked from in there, why the tree is walked inside the bar's column, and why
  whether the browser was showing is captured once per root's frame.
- `interface.c` — one `ui` frame per root, submitted into the open frame, and
  the one read of the frame's clicks that carries out the top bar's, the
  browser's and Preferences' commands.
- `inspector.h` — what the selected entity is made of, and the controls that
  change it; an edit is a replace intent and never a write, and how many were
  submitted this frame is a count `main.c` reads. Its header says why the
  controls and every label's text have to outlive the call that drew them.
- `inspector.c` — the walk over the world's component types driven by
  base/describe.h alone, a wrapping row per described field, the three angles
  shown and never stored, and the replace intent a moved control becomes.
- `view.h` — a scene view: its camera, its target and the middle-button drag.
  Its header says why the camera is not an entity, why the light is the
  world's, why the picture's size lags the layout by a frame, and why a view
  whose leaf is not in the tree is not drawn.
- `view.c` — the views' orbit, which owns the eye, the drag's rates per
  millimetre, and their targets.
- `scene.h` — the current project's world, the selection in it, the rows the
  Scene panel drew, and what the Inspector drew this frame. Its header says why
  building a project's entities is not this file's job, why the selection is
  the editor's and not the dock tree's, and why those rows outlive the call
  that drew them.
- `scene.c` — the selection, and the one question asked of the Scene panel's
  rows after the frame has ended; nothing in it draws or lays out.
