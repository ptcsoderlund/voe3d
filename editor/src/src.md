# src

`editor`'s implementation. There is no public header: `main.c` is the program,
and every other file is a `.h` saying what it owns and why beside the `.c` that
carries it out.

- `main.c` — opens a project, the window and the device, makes the arena, font, themes and
  interface, shows the starting line on the splash while render prepares, uploads the shapes, opens
  the views, runs the loop until a close goes ahead or the picture is written, and logs the start.
- `splash.h` — the engine's splashscreen.png, never a project's, read from the engine source at run
  time so a missing copy shows the plain screen, kept by the caller until the device closes.
- `splash.c` — the path joined from toolchain.h's engine folder and read with app/picture.h.
- `frame_commands.h` — the loop's keyboard commands: the shortcuts read against main.c's guards,
  the acts on them, Escape's order, the undo step taken next frame, and the acts after the draw.
- `frame_commands.c` — the history step, the read with its acts and `ui`'s keyboard, and Delete,
  Ctrl+D, R and the edit marked after the interface has drawn.
- `startup.h` — which project the editor opens on: the argued folder, the remembered one or
  untitled, written back as the last project unless capturing, and the descriptions line.
- `startup.c` — the three tried in order, what each failure says, and the last-project write.
- `world_step.h` — the world's step once a frame, `game`'s step whose order is now `game`'s, why
  the editor runs no move but runs emitters by the frame's seconds, and why placed copies are
  expanded after it.
- `world_step.c` — the one call to `voe_game_world_step`, the emitters' and the water's runs by the
  seconds, the point lights' replaces with none, then the expansion.
- `prefabs.h` — what the editor does with prefabs: every placed copy expanded from its file in
  ascending id, a load, with deterministic ids and a file that will not read said in the notice;
  a tree dragged into the Assets panel made one, what refuses it, and the refusals asked per frame.
- `prefab_expand.c` — the next unexpanded root found, its file read onto it, the ids carried, and a
  refused root given its part row.
- `prefab_make.c` — a tree's camera, light or prefab rows refused, and any while a prefab is open,
  those refusals as one call without the file test, the file written under the shown folder unless
  it exists, then the prefab and part rows queued.
- `capture.h` — `--capture`'s drawn frames counted to `--frames`, two by default, and the window's
  picture and a scene view's own target written as PNGs.
- `capture.c` — the count and the two writes, each through its own scratch arena.
- `keys.h` — this frame's keyboard: the level `platform` reports for every key and the down edge of
  each against last frame's, which is where every key edge in the program is found.
- `keys.c` — the one read of every key, once a frame, and the levels it
  remembers to find the next frame's edges against.
- `shortcuts.h` — what this frame's keyboard asked the editor to do: a flag per shortcut, worked out
  once out of keys.h's frame and the guards the caller holds, a flying view silencing all of them,
  with acting on one left to the caller.
- `shortcuts.c` — the one read of those flags: the three Ctrl commands, Delete, Ctrl+D and R, the
  rest a step is recorded at with Ctrl+Z and Ctrl+Y on it, and Escape's raw and free edges.
- `options.h` — the command line, as UTF-8 arguments from `platform`: the folder to open,
  `--capture`'s path, `--size`'s picture, `--frames`' count and `--capture-view`'s path, or one
  usage line on stderr.
- `options.c` — the argument list walked once, the size parsed as two runs of
  digits with an `x` between, and the one usage line every mistake ends at.
- `project.h` — the project being worked on: its three arenas, kept sections, project file with its
  game window, code, folder and unsaved flag (ADR-0164), where its worlds are made, its scene as
  text or read back and expanded, the new-code swap, and a prefab opened with the level set aside.
- `project.c` — opening, making and saving a project or its open prefab, the game window set and
  written at once, the scene written out as text and read back into the same world or a new one,
  each read expanded, a prefab opened and Back, and the untitled scene's cube, light and camera.
- `last_project.h` — the one remembered folder at
  `<settings>/voe3d/last_project`. Its header says why a first start is not a
  failure worth reporting.
- `last_project.c` — reading that file as its one line and writing it by making
  the two folders above it as needed.
- `settings.h` — the Scene list's and the Inspector's widths, the Assets panel's and the top bar's
  heights, the views' share and the four dock panels' open flags a person left, remembered at
  `<settings>/voe3d/editor_settings`, one `<key> <value>` line each (ADR-0226, 0363).
- `settings.c` — that file read line by line as a key and a number in range, and written back with
  every other key's line kept, by making the two folders above it as needed.
- `themes.h` — Near black and Near white, then one theme per `*.theme` file, the chosen one
  remembered and re-read once a second, each derived at the editor's base text size and spacing.
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
- `game_tree.h` — what Play and Ship write and run: `<project>/Build/game/`'s four files, the
  `.gitignore`, the argument lists that build the tree as the game, the library or a release, and
  install it into the shipped folder `Build/ship/<name>/`, and the paths it reads and writes.
- `game_tree.c` — the files compared before they are written, the name and engine path escaped for
  where they go, the game window's numbers in main.c, the world cooked into scene.c, every prefab
  found and cooked into prefabs.c, the release kind and the shipped folder.
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
- `session.h` — the project being worked on, its Play, Refresh, Ship and open prefab, the Errors
  panel shown from the build log, and the armed command that makes Close, New, Open and Back
  refuse once over unsaved work.
- `session.c` — the refuse-once rule, the eight commands with Play and Ship refreshing first, one
  build at a time and refused while a prefab is open, a prefab opened, a built library swapped in
  once a frame, what a browser action does to the session, and the load a frame later.
- `topbar.h` — the bar across the top: Back while a prefab is open, the project's commands,
  Project, Preferences, Panels and its list, the gizmo's mode, the name and unsaved mark, then the
  notice, as tall as its content or as the person made it.
- `topbar.c` — the bar's one frame of `ui` calls, a panel holding one row, and
  the read of its buttons, Back, Play, Refresh, Ship and Project among them, afterwards.
- `panels_menu.h` — the list Panels opens: one row per closable panel, an open one ticked with √ in
  a column of its own, as an anchored panel below the button, closed by its caller.
- `panels_menu.c` — the list's one frame of `ui` calls and the read of its rows, and whether the
  pointer is on it, afterwards.
- `project_panel.h` — the Project panel: a title row with its ×, the game window's width and height
  as number boxes and a Windowed / Fullscreen choice, as an anchored panel over the dock, carrying
  out nothing.
- `project_panel.c` — the panel's one frame of `ui` calls and the read of its controls afterwards,
  a number rounded and clamped to the project file's range.
- `preferences.h` — Preferences: one row per theme with its name and a Choose button, the one in
  force marked, three sliders for that theme's contrast, separation and text size with a Reset
  button, and Close, as an anchored panel over the dock.
- `preferences.c` — the panel's one frame of `ui` calls and the read of its
  buttons and sliders afterwards.
- `errors.h` — the Errors panel a failed build shows: the last 48 lines of `Build/build.log`, each
  cut at 160 bytes, in a scroll area under a title row with its ×, as an anchored panel over the dock.
- `errors.c` — the log read back from its end to its last lines, the panel's one frame of `ui`
  calls, and the read of its × afterwards.
- `browser.h` — the editor's own file browser: a folder listing as an anchored panel over the dock,
  its own arena, a start beside a given folder with its row chosen, in SAVE mode a name row with a
  focused `ui` field and Make folder, and in IMPORT mode `.glb` files a press imports.
- `browser.c` — the browser's listing, its one frame of `ui` calls, and the read
  of its buttons and rows afterwards.
- `dock.h` — the tree of four panels, Scene over Assets on the left, whose splits hold a side
  panel's length or the views' share, the closable panels with a closed leaf not laid out, every
  seam's place (the reached one lit), whether a point is over a panel, and the walk.
- `dock.c` — the tree: the default one, the arrangement, held lengths and the views' share clamped
  and set, which views it shows and whether a point is over a panel.
- `dock_walk.c` — the walk to one frame of `ui` calls with its seams, each closable leaf's header of
  name and ×, each leaf a panel and scroll area handed to its panel's draw with the palette for the Scene list's drag marks, the Inspector's
  call, and each scene view's camera preview.
- `scene_list.h` — the Scene list: heading, Add entity and one row per authored entity as a tree, a
  row with children folding by its identity's saved flag, a copy marked with its prefab's file; a
  drag parents, unparents or, onto the Assets panel, makes a prefab; a part never drags.
- `scene_list.c` — the list's one frame of `ui` calls, walking the parent tree with a capped
  stack, rows tightly padded in one gapless column, each in a rim wrapper, a copy's file name,
  the drag's ghost refused or not, and the held row's drop.
- `drag_ghost.h` — the ghost every editor drag shows beside the pointer: a raised panel of the
  dragged thing's name, dimmed with "Can't drop here" when a release would drop nothing.
- `drag_ghost.c` — the dim pushed when refused, the anchored panel, the name and the second line.
- `assets_panel.h` — the Assets panel: `<project>/Assets/` as rows in its own arena, folders
  entered and Up but never above it, listed again once a second, a fired prefab row left to open,
  and Import.
- `assets_panel.c` — the project's and the shown folder's listings, the rows filled in two passes,
  the panel's one frame of `ui` calls, the read of its rows, a fired prefab's path, Up and Import
  afterwards, and the import.
- `assets_drag.h` — a held model, prefab or picture row from the Assets panel: released over a
  scene view it places a new thing or a copy where the ray lands, over the Inspector it swaps the
  selected thing's model or its emitter's texture, elsewhere nothing.
- `assets_drag.c` — the drag started from the panel's held row, the one outcome at a pointer for
  the release and the ghost, the drop point, the Inspector's rectangle, the texture swap, and the
  one undo step.
- `resize.h` — the borders a person drags to size the panels: each side panel's seam, the Assets
  panel's, the views' and the top bar's lower edge, hit-tested before `ui`, the pointer's shape, the
  reached seam for the walk to light, and a double-click setting one size back (ADR-0226).
- `resize.c` — the hover, press, drag and release against the tree laid out below the bar, the
  double-click timed on the caller's clock.
- `panels.h` — the editor's panels as the person left them: the default tree started with the
  settings file read over it, the sizes remembered back into it when a drag ends, the toggle that
  opens or closes a dock panel, Project or Errors, and whether one is open.
- `panels.c` — the default tree's sizes filled in, read over and set back, and the same fields
  written through `settings.h`.
- `interface.h` — the screen-filling surface, made in the theme it is handed: pixels per millimetre
  from the window's height, the top bar above each root's dock tree, the browser, Preferences,
  Project or the colour picker over it, and one draw command per root.
- `interface.c` — one `ui` frame per root, and the one read of the frame's clicks where the
  commands they fire are carried out.
- `inspector.h` — what the selected entity is made of, the controls that change it, and the struct
  one frame of them is recorded in; it holds the shape of the open dropdown, because this panel
  draws that list and reads what was picked from it; a prefab's part is shown, never edited.
- `inspector.c` — the Duplicate and Delete row, the walk over described component types, each
  section's heading, Remove and "Needs" line, a row per field, each control's record, Add component
  and its menu, the open list, and a part shown read-only.
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
  open dropdown and Add component's lists and submenus opened, closed and placed each frame; none
  on a prefab's part.
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
- `view_passes.h` — what a frame draws into the views: a pass per shown view after its shadow
  passes, lit by every light, with the world, its models, the selection's outline, collider,
  blocker box and gizmo, the markers, and the device capacities.
- `view_passes.c` — the preview's pass while the selected entity has a camera, then each shown
  view's shadow passes and pass, lit by every directional and point light and kept out of the light
  blockers,
  stopping at the first refused pass.
- `models.h` — the editor's one model store: loaded from the project folder, re-read once a second,
  emptied on a different project, a broken file said in the notice, and handed to picking and the
  view passes.
- `models.c` — the store made, emptied on a new folder, filled and re-read through game/models.h
  with a failure's notice, cleared through the device and destroyed.
- `scene.h` — the current project's world, the selection in it, the rows the Scene panel drew, Add
  entity, Delete and Duplicate, what the Inspector drew, what the colour picker and the open
  dropdown are open on, the gizmo's unsaved mode, and the Scene list drag's threshold, target and
  cancel.
- `scene.c` — the selection, Delete and Duplicate, the gizmo's switch, the colour picker and
  dropdown targets, and the Scene panel's rows and folds, asked after the frame has ended.
- `pick.h` — a left click in a scene view selects the frontmost entity under the pointer, a model's
  too, and a click on nothing clears the selection; the ray and what it meets are `3d`'s (ADR-0202).
- `pick.c` — the press edge, the view the pointer is over, the ray through that
  view's picture, and the selection set from whatever it met.
- `gizmo.h` — what the primary button does to the selected entity's gizmo, arrows or rings: the
  handle under the pointer, a press that grabs one, and the drag that submits its new position or
  rotation; a part has none.
- `gizmo.c` — the hover, the grab and the move or turn, each against a gizmo built from the view's
  own camera, measured from the press in world space and submitted as a whole transform, a child's
  written back relative to its parent.
- `undo.h` — the line of scene texts a step is taken from: an edit marked, a
  settled edit recorded as the whole scene's text, Ctrl+Z or Ctrl+Y reading
  a neighbouring one back into the project's world (ADR-0204), and the level's
  line set aside while a prefab is open.
- `undo.c` — the states pushed once, the two lines swapped, the compare a settled edit makes against
  the state the world is, the throwing away of what could have been redone, and
  the selection re-found by its authored id after a step.
- `entities.h` — Add entity, a dropped model's thing or prefab's copy, duplicating and deleting
  entities with their trees, and giving or taking components, all through the world's structural
  queue. Its header says the id and name rules.
- `entities.c` — the identity room check before a make, the new id and name, the queued rows, a
  tree's destroys, and the destroy that undoes a half-made entity.
