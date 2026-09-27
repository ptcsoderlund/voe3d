# src

`editor`'s implementation. There is no public header: `main.c` is the program,
and every other file is a `.h` saying what it owns and why beside the `.c` that
carries it out.

- `main.c` — opens a project, the window and the device, makes the arena, font, themes and
  interface, uploads the shapes, opens the views on the scene's camera, and runs the loop until a
  close goes ahead or the picture is written.
- `startup.h` — which project the editor opens on: the argued folder, the remembered one or
  untitled, written back as the last project unless capturing, and the descriptions line.
- `startup.c` — the three tried in order, what each failure says, and the last-project write.
- `world_step.h` — the world's step once a frame, `game`'s step whose order is now `game`'s, and
  why the editor runs no move.
- `world_step.c` — the one call to `voe_game_world_step`.
- `capture.h` — `--capture`'s drawn frames counted to two and the window's picture written as a PNG.
- `capture.c` — the count and the one write through its own scratch arena.
- `keys.h` — this frame's keyboard: the level `platform` reports for every key and the down edge of
  each against last frame's, which is where every key edge in the program is found.
- `keys.c` — the one read of every key, once a frame, and the levels it
  remembers to find the next frame's edges against.
- `shortcuts.h` — what this frame's keyboard asked the editor to do: a flag per shortcut, worked out
  once out of keys.h's frame and the guards the caller holds, a flying view silencing all of them,
  with acting on one left to the caller.
- `shortcuts.c` — the one read of those flags: the three Ctrl commands, Delete, Ctrl+D and R, the rest
  a step is recorded at with Ctrl+Z and Ctrl+Y on it, and Escape's raw and free edges.
- `options.h` — the command line, as UTF-8 arguments from `platform`: the folder to open,
  `--capture`'s path and `--size`'s picture, or one usage line on stderr.
- `options.c` — the argument list walked once, the size parsed as two runs of
  digits with an `x` between, and the one usage line every mistake ends at.
- `project.h` — the project being worked on: its own arena, the world's and a scene read's, the kept
  sections, the code its world was made with, its folder and whether it has unsaved changes
  (ADR-0164), its scene handed out as text or read back in, and the swap to a world with new code.
- `project.c` — opening, making and saving a project, the scene written out as text and a text read
  back into the same world or a new one made with new code, and the untitled scene's cube, light and
  camera, which are this file's decision and not `scene.c`'s.
- `last_project.h` — the one remembered folder at
  `<settings>/voe3d/last_project`. Its header says why a first start is not a
  failure worth reporting.
- `last_project.c` — reading that file as its one line and writing it by making
  the two folders above it as needed.
- `settings.h` — the Scene list's and the Inspector's widths, the top bar's height and the views'
  share a person gave them, remembered at `<settings>/voe3d/editor_settings`, one `<key> <number>`
  line each (ADR-0226).
- `settings.c` — that file read line by line as a key and a number in range, and written back with
  every other key's line kept, by making the two folders above it as needed.
- `themes.h` — Near black and Near white, then one theme per `*.theme` file in
  `<settings>/voe3d/themes/`, the chosen one remembered in `<settings>/voe3d/theme` and re-read once
  a second, each carrying the two scalars and the text scale it is drawn with.
- `themes.c` — the folder listed and made, each file read and derived with the one font, the
  remembered file read and written as its one line, the chosen file's once-a-second re-read, and
  each theme derived at its own text scale.
- `theme_scalars.h` — a person's contrast, surface separation and text scale remembered per
  theme at `<settings>/voe3d/theme_scalars`, one line per adjusted theme (ADR-0197, ADR-0224).
- `theme_scalars.c` — that file read line by line as three numbers and the name
  after them, a theme's numbers set or forgotten in the list, and every line
  written back by making the two folders above it as needed.
- `notice.h` — one line long enough to explain why a project failed to open or
  save. Its header says what a caller has to do before asking for one built
  from base/report.h's first kept error.
- `notice.c` — a notice's text cleared, set from a format, or built from
  base/report.h's first kept error.
- `code.h` — a project's library loaded from a copy under `Build/editor/loaded/`, its one entry
  point resolved (ADR-0008), whether a build equals it, and why it closes after its worlds.
- `code.c` — the folders made, the copy read and written, the open and the lookup, and the compare.
- `game_tree.h` — what Play and Ship write and run: `<project>/Build/game/`'s three files, the
  `.gitignore`, the argument lists that build the tree as the game, the library or a release, and
  install it into the shipped folder `Build/ship/<name>/`, and the paths it reads and writes.
- `game_tree.c` — the files compared before they are written, the name and engine path escaped for
  where they go, the world cooked into scene.c, each argument list in one struct, the release kind
  and the shipped folder.
- `play.h` — Play: the game tree written, configured and built as needed, then the game started as a
  program of its own, a second press ending it, and the label Play, Building or Stop.
- `play.c` — the tree written and the first step started at the press, each ended step polled on
  to the next or to one stderr line naming the build log, and the play's arena destroyed whenever
  it goes idle.
- `refresh.h` — Refresh: the game tree configured and built as the project's library, a step at a
  time without blocking, a failed step one stderr line naming the build log, and the label.
- `refresh.c` — the tree written and the first step started, each ended step polled on to the build,
  BUILT or FAILED, and the refresh's arena destroyed whenever it goes idle.
- `ship.h` — Ship: the game tree configured and built in release, the old shipped folder cleared
  and the game installed into it, a step at a time without blocking, a failed step one stderr line
  naming the build log, and the label.
- `ship.c` — the tree written and the first step started, each ended step polled on to the next,
  SHIPPED with the shipped folder's path or FAILED, and the ship's arena destroyed whenever it goes
  idle.
- `session.h` — the project being worked on, its notice, its Play, Refresh and Ship, the flag that
  says a different project is in place, and the one armed command that makes closing the window,
  New and Open each refuse once while there are unsaved changes and go ahead the second time.
- `session.c` — the refuse-once rule, the seven commands with Play and Ship refreshing first and one
  build at a time, a built library loaded and swapped in once a frame, and what a browser action
  does to the session.
- `topbar.h` — the bar across the top of the root surface: New, Open, Save, Play, Refresh, Ship,
  Preferences, the gizmo's mode, the project's name and whether it is unsaved, then the session's notice, at least as
  tall as its content measured last frame and as tall as the person made it.
- `topbar.c` — the bar's one frame of `ui` calls, a panel holding one row, and
  the read of its seven buttons, Play, Refresh and Ship among them, afterwards.
- `preferences.h` — Preferences: one row per theme with its name and a Choose button, the one in
  force marked, three sliders for that theme's contrast, separation and text size with a Reset
  button, and Close, as an anchored panel over the dock.
- `preferences.c` — the panel's one frame of `ui` calls and the read of its
  buttons and sliders afterwards.
- `errors.h` — the Errors panel a failed build shows: the last 48 lines of `Build/build.log`, each
  cut at 160 bytes, in a scroll area with Close, as an anchored panel over the dock.
- `errors.c` — the log read back from its end to its last lines, the panel's one frame of `ui`
  calls, and the read of Close afterwards.
- `browser.h` — the editor's own file browser: a folder listing shown as an anchored panel over the
  dock, its own arena for the current folder and its rows, in SAVE mode a name row with a focused
  `ui` field and a Make folder button, and in IMPORT mode `.glb` files a press imports.
- `browser.c` — the browser's listing, its one frame of `ui` calls, and the read
  of its buttons and rows afterwards.
- `dock.h` — the tree of four panels, Scene over Assets on the left, whose splits hold a side panel's length in millimetres or the views' share,
  every node's and seam's place (seams in the border colour, the reached one lit), the walk and
  `voe_editor_panel_draw`; a root carries this frame's keyboard and lit seam beside its pointer.
- `dock.c` — the arrangement, a held length or the views' share clamped to what each side needs,
  the walk to one frame of `ui` calls, the held lengths and share read and set, the Inspector's
  scroll area, and the camera preview in each scene view's corner.
- `assets_panel.h` — the Assets panel: `<project>/Assets/` as rows, folders first, entered and gone
  Up from but never above `Assets/`, listed again once a second in its own arena, `.glb` rows marked
  as models, and Import copying a chosen `.glb` into the shown folder.
- `assets_panel.c` — the project's and the shown folder's listings, the rows filled in two passes,
  the panel's one frame of `ui` calls, the read of its rows, Up and Import afterwards, and the import.
- `resize.h` — the borders a person drags to size the panels: each side panel's seam, the views' border and the top
  bar's lower edge, hit-tested before `ui`, the pointer's shape over them, the seam it reports reached
  for the walk to light, and a double-click that sets one size back (ADR-0226).
- `resize.c` — the hover, press, drag and release against the tree laid out below the bar, the
  double-click timed on the caller's clock, and the three sizes written through `settings.h`.
- `interface.h` — the screen-filling surface, made in the theme it is handed: pixels per millimetre
  from the window's height, the top bar above each root's dock tree, the browser, Preferences or the
  colour picker over it, and one draw command per root.
- `interface.c` — the play state and the ship polled once a frame, one `ui` frame per root,
  submitted into the open frame, and the one read of the frame's clicks that carries out the top
  bar's, Play and Ship among them, the browser's and Preferences' commands and the colour picker's
  changes.
- `inspector.h` — what the selected entity is made of, the controls that change it, and the struct
  one frame of them is recorded in; it holds the shape of the open dropdown, because this panel
  draws that list and reads what was picked from it.
- `inspector.c` — the Duplicate and Delete row, the walk over the world's described component types,
  each section's heading, Remove and "Needs" line, a wrapping row per described field, the record
  each control leaves behind, the Add component button and its menu, and the open list.
- `add_menu.h` — the entries Add component offers one entity, a tree of groups and types built each
  frame from the types' registered menu paths (ADR-0217, 0221), and each level drawn as a list.
- `add_menu.c` — each offered type's path split on `/` and trimmed, its groups found or made under
  their parent, its entry appended in registration order, and one parent's children drawn as a list
  the way the open dropdown's is.
- `inspector_edit.h` — the calls that turn what the pointer did to the Inspector's controls into
  replace intents and scene.h's commands, and the rules for when each open list closes.
- `inspector_edit.c` — a dragged or typed number submitted as the component's replace intent, a
  rotation's edit as the difference about a world axis, a committed text field as the row's CHAR
  bytes, and the picker's colour and the open list's choice submitted the same way.
- `inspector_buttons.c` — the fired buttons carried out through scene.h and entities.h, and the
  open dropdown and Add component's lists and submenus opened, closed and placed each frame.
- `inspector_place.h` — the side-and-cap rule the open lists and submenus are placed by, and the
  rectangle test a press outside them is.
- `inspector_place.c` — a list fitted below, above or capped on the roomier side, under its button
  or beside its row, in the content column's space.
- `inspector_value.h` — what a field's bytes say: a kind and an offset in, a number, three shown
  angles or the one string a label is given out.
- `inspector_value.c` — a number read out of a field's bytes whatever its width, the Z-Y-X
  decomposition of a rotation, a type's heading from its key, a field as one string, and how many
  boxes a kind is worth.
- `entity_field.h` — what an ENTITY field offers and says: None and the authored entities by name,
  and why names and only authored ones.
- `entity_field.c` — the identity table sorted by id into the choices, and a name or "None" as the
  label.
- `view.h` — a scene view: its orbit's pose and lens, its target, the middle-button drag and the
  right-button fly, the view a pointer is over and where in its picture, the colours a view is drawn
  with, the 480×270 preview of what the world's camera sees, and the views opened on that camera.
- `view.c` — the views' orbit, which owns the eye, the drag's and the fly's rates, their targets,
  the focus set to the world camera's position, the world's first light row and the selection's
  outline colour, dimmed for a gizmo handle at rest.
- `view_passes.h` — what a frame draws into the views: a pass per shown view, after its shadow passes, with the world and its models, the
  selection's outline, its collider, its gizmo and the camera's and sun's markers, and the device capacities those passes need.
- `view_passes.c` — the preview's pass while the selected entity has a camera, then the shown views
  walked in order, each one's shadow passes then its pass begun, drawn by `3d`'s draw system with the world's camera and sun marked, and
  ended, stopping at the first refused pass.
- `models.h` — the editor's one model store: loaded from the project folder, re-read once a second,
  emptied on a different project, a broken file said in the notice, and handed to picking and the
  view passes.
- `models.c` — the store made, emptied on a new folder, filled and re-read through game/models.h
  with a failure's notice, cleared through the device and destroyed.
- `scene.h` — the current project's world, the selection in it, the rows the Scene panel drew, its
  Add entity button, Delete and Duplicate, the structural changes made this frame, what the Inspector
  drew, what the colour picker and the open dropdown are open on, and the gizmo's unsaved mode.
- `scene.c` — the selection, Delete and Duplicate (both refuse the camera, Duplicate the sun), the
  gizmo's switch, the colour picker and dropdown opened, closed and placed where the Inspector
  measured them, and the Scene panel's rows asked after the frame has ended.
- `pick.h` — a left click in a scene view selects the frontmost entity under the pointer, a model's
  too, and a click on nothing clears the selection; the ray and what it meets are `3d`'s (ADR-0202).
- `pick.c` — the press edge, the view the pointer is over, the ray through that
  view's picture, and the selection set from whatever it met.
- `gizmo.h` — what the primary button does to the selected entity's gizmo, arrows or rings: the
  handle under the pointer, a press that grabs one, and the drag that submits its new position or
  rotation.
- `gizmo.c` — the hover, the grab and the move or turn, each against a gizmo built from the view's
  own camera, measured from the press and submitted as a whole transform.
- `undo.h` — the line of scene texts a step is taken from: an edit marked, a
  settled edit recorded as the whole scene's text, and Ctrl+Z or Ctrl+Y reading
  a neighbouring one back into the project's world (ADR-0204).
- `undo.c` — the states pushed once, the compare a settled edit makes against
  the state the world is, the throwing away of what could have been redone, and
  the selection re-found by its authored id after a step.
- `entities.h` — Add entity (an identity and a transform, nothing else), duplicating and deleting
  entities and giving or taking their components, a new collider fitting the shape, all through the
  world's structural queue. Its header says
  the id and name rules and what a failure leaves behind.
- `entities.c` — the new id and name, the queued rows, and the destroy that
  undoes a half-made entity.
