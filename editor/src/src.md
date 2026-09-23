# src

`editor`'s implementation. There is no public header: `main.c` is the program,
and every other file is a `.h` saying what it owns and why beside the `.c` that
carries it out.

- `main.c` — reads the command line, opens a project, the window and the device, makes the arena,
  the one font, the themes and the interface, uploads the built-in shapes, and runs the loop until a
  close goes ahead or the picture is written.
- `keys.h` — this frame's keyboard: the level `platform` reports for every key and the down edge of
  each against last frame's, which is where every key edge in the program is found.
- `keys.c` — the one read of every key, once a frame, and the levels it
  remembers to find the next frame's edges against.
- `shortcuts.h` — what this frame's keyboard asked the editor to do: a flag per shortcut, worked out
  once out of keys.h's frame and the guards the caller holds, with acting on one left to the caller.
- `shortcuts.c` — the one read of those flags: the three Ctrl commands, Delete and Ctrl+D, the rest
  a step is recorded at with Ctrl+Z and Ctrl+Y on it, and Escape's raw and free edges.
- `options.h` — the command line: the folder to open, `--capture`'s path and
  `--size`'s picture, or one usage line on stderr.
- `options.c` — the argument list walked once, the size parsed as two runs of
  digits with an `x` between, and the one usage line every mistake ends at.
- `project.h` — the project being worked on: its own arena, the arena a scene read owns, the world
  in it, the kept sections it was read with, its absolute folder and whether it has unsaved changes
  (ADR-0164), and its scene handed out as text or read back in from one.
- `project.c` — opening, making and saving a project, the scene written out as
  text and a text read back into the same world, the one `world_new` every
  world is built by, and the untitled scene's cube and light, which are this
  file's decision and not `scene.c`'s.
- `last_project.h` — the one remembered folder at
  `<settings>/voe3d/last_project`. Its header says why a first start is not a
  failure worth reporting.
- `last_project.c` — reading that file as its one line and writing it by making
  the two folders above it as needed.
- `themes.h` — Near black and Near white, then one theme per `*.theme` file in
  `<settings>/voe3d/themes/`, the chosen one remembered in `<settings>/voe3d/theme` and re-read once
  a second, each carrying the two scalars it is drawn with.
- `themes.c` — the folder listed and made, each file read and derived with the one font, the
  remembered file read and written as its one line, and the chosen file's once-a-second re-read.
- `theme_scalars.h` — a person's contrast and surface separation remembered per theme at
  `<settings>/voe3d/theme_scalars`, one line per adjusted theme (ADR-0197).
- `theme_scalars.c` — that file read line by line as two numbers and the name
  after them, a theme's numbers set or forgotten in the list, and every line
  written back by making the two folders above it as needed.
- `notice.h` — one line long enough to explain why a project failed to open or
  save. Its header says what a caller has to do before asking for one built
  from base/report.h's first kept error.
- `notice.c` — a notice's text cleared, set from a format, or built from
  base/report.h's first kept error.
- `session.h` — the project being worked on, its notice, the flag that says a different project is in
  place, and the one armed command that makes closing the window, New and Open each refuse once while
  there are unsaved changes and go ahead the second time.
- `session.c` — the refuse-once rule, the four commands, and what a browser
  action does to the session.
- `topbar.h` — the bar across the top of the root surface: New, Open, Save, Preferences, the
  project's name and whether it is unsaved, then the session's notice.
- `topbar.c` — the bar's one frame of `ui` calls, a panel holding one row, and
  the read of its four buttons afterwards.
- `preferences.h` — Preferences: one row per theme with its name and a Choose button, the one in
  force marked, a slider for each of that theme's two scalars with a Reset button, and Close, as an
  anchored panel over the dock.
- `preferences.c` — the panel's one frame of `ui` calls and the read of its
  buttons and sliders afterwards.
- `browser.h` — the editor's own file browser: a folder listing shown as an anchored panel over the
  dock, its own arena for the current folder and its rows, and in SAVE mode a name row with a
  focused `ui` field and a Make folder button.
- `browser.c` — the browser's listing, its one frame of `ui` calls, and the read
  of its buttons and rows afterwards.
- `dock.h` — the tree, the walk, and `voe_editor_panel_draw`, which is where the panels' contents
  are; a root carries this frame's keyboard beside its pointer.
- `dock.c` — the walk from a tree of nodes to one frame of `ui` calls, the one-millimetre gap at
  each seam where a splitter would go, and the scroll area it opens for the Inspector's leaf.
- `interface.h` — the screen-filling surface, made in the theme it is handed: pixels per millimetre
  from the window's height, the top bar above each root's dock tree, the browser, Preferences or the
  colour picker over it, and one draw command per root.
- `interface.c` — one `ui` frame per root, submitted into the open frame, and the one read of the
  frame's clicks that carries out the top bar's, the browser's and Preferences' commands and the
  colour picker's changes.
- `inspector.h` — what the selected entity is made of, the controls that change it, and the struct
  one frame of them is recorded in; it holds the shape of the open dropdown, because this panel
  draws that list and reads what was picked from it.
- `inspector.c` — the Duplicate and Delete row, the walk over the world's described component types,
  each section's heading, Remove and "Needs" line, a wrapping row per described field, the record
  each control leaves behind, the Add component button and its menu, and the open list.
- `add_menu.h` — the entries Add component offers one entity, a tree of groups and types built each
  frame from the types' registered menu paths (ADR-0217, 0221), and its top level drawn as a list.
- `add_menu.c` — each offered type's path split on `/` and trimmed, its groups found or made under
  their parent, its entry appended in registration order, and the list drawn as the open dropdown's is.
- `inspector_edit.h` — the calls that turn what the pointer did to the Inspector's controls into
  replace intents and scene.h's commands, and the side-and-cap rule both open lists are placed by.
- `inspector_edit.c` — a dragged or typed number submitted as the component's replace intent, a
  rotation's edit as the difference about a world axis, a committed text field as the row's CHAR
  bytes, the fired buttons, Add component's list opened and closed, and the open lists' side and cap.
- `inspector_value.h` — what a field's bytes say: a kind and an offset in, a number, three shown
  angles or the one string a label is given out.
- `inspector_value.c` — a number read out of a field's bytes whatever its width, the Z-Y-X
  decomposition of a rotation, a type's heading from its key, a field as one string, and how many
  boxes a kind is worth.
- `view.h` — a scene view: its camera, its target and the middle-button drag, which view a pointer
  is over and where in that view's picture it lands, and the light, the outline colour and the
  gizmo's colour a view is drawn with.
- `view.c` — the views' orbit, which owns the eye, the drag's rates per
  millimetre, their targets, the world's first light row and the lighter half
  of the theme's inverse pair the selection is outlined in, dimmed for a gizmo handle at rest.
- `scene.h` — the current project's world, the selection in it, the rows the Scene panel drew, its
  Add entity button, Delete and Duplicate, the structural changes made this frame, what the Inspector drew,
  and what the colour picker and the open dropdown are open on.
- `scene.c` — the selection, Delete and Duplicate, opening and closing the colour picker and the
  dropdown, the open list moved to where the Inspector measured it, and the one question asked of
  the Scene panel's rows after the frame has ended.
- `pick.h` — a left click in a scene view selects the frontmost entity under the pointer, and a
  click on nothing clears the selection; the ray and what it meets are `3d`'s (ADR-0202).
- `pick.c` — the press edge, the view the pointer is over, the ray through that
  view's picture, and the selection set from whatever it met.
- `gizmo.h` — what the primary button does to the selected entity's move gizmo: the handle under
  the pointer, a press that grabs one, and the drag that submits the entity's new position.
- `gizmo.c` — the hover, the grab and the drag, each against a gizmo built from the view's own
  camera, measured from the press position and submitted as a whole transform.
- `undo.h` — the line of scene texts a step is taken from: an edit marked, a
  settled edit recorded as the whole scene's text, and Ctrl+Z or Ctrl+Y reading
  a neighbouring one back into the project's world (ADR-0204).
- `undo.c` — the states pushed once, the compare a settled edit makes against
  the state the world is, the throwing away of what could have been redone, and
  the selection re-found by its authored id after a step.
- `entities.h` — Add entity (an identity and a transform, nothing else), duplicating and deleting
  entities and giving or taking their components, all through the world's structural queue. Its header says
  the id and name rules and what a failure leaves behind.
- `entities.c` — the new id and name, the queued rows, and the destroy that
  undoes a half-made entity.
