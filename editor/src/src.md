# src

`editor`'s implementation. There is no public header: `main.c` is the program,
and every other file is a `.h` saying what it owns and why beside the `.c` that
carries it out.

- `main.c` — opens the project, window and device, starts and loads scenes on a worker behind a
  live splash, and runs the loop until a close goes ahead or the picture is written, taking the
  frame breakdown's timings each frame and writing, saving and reverting landscapes.
- `loading.h` — the start's and a New or Open load's works for a splash wait: shaders with the
  editor's pipeline cache, shapes and models, or the scene and its models, and what the worker owns.
- `loading.c` — the two works, each step after a stop check, in the worker's own scratch.
- `splash.h` — the engine's splashscreen.png, never a project's, read from the engine source at run
  time so a missing copy shows the plain screen, kept by the caller until the device closes.
- `splash.c` — the path joined from toolchain.h's engine folder and read with app/picture.h.
- `frame_commands.h` — the loop's keyboard commands: the shortcuts read against main.c's guards,
  the acts on them, Escape's order, the undo step taken next frame, and the acts after the draw.
- `frame_commands.c` — the history step closing the open material on a new project, the read with its acts (F2's rename, an asset's Delete,
  Escape choosing no brush after the lists and hiding the Landscape panel) and `ui`'s keyboard, and
  Delete, Ctrl+D, R, the edit and a reveal's unfold marked after the interface has drawn.
- `frame_pointer.h` — the loop's pointer and view reads in their order: the fly, the shortcuts, the
  borders, the middle drag, the brush, the gizmo, the Assets drag and the pick, and what main.c
  reads after.
- `frame_pointer.c` — the fly with its lock edge, the borders with their clears and remember, and
  the brush then the three left-press readers under one shared block a stroke joins.
- `frame_selection.h` — F: the view under the pointer glides to the selection's size from `3d`, or
  to a sizeless thing's place at a fixed 3 m, nothing saved or undone.
- `frame_selection.c` — the selection and view checked, the bounds or world position, the distance
  and the glide.
- `startup.h` — which project the editor opens on: the argued folder, the remembered one or
  untitled, written back as the last project unless capturing, and the descriptions line.
- `startup.c` — the three tried in order, what each failure says, and the last-project write.
- `world_step.h` — the world's step once a frame, the transforms remembered first for the bounce's
  stale spheres, `game`'s step whose order is now `game`'s, why the editor runs no move but runs emitters by the frame's seconds, and why placed copies are
  expanded after it.
- `world_step.c` — the remember, the one call to `voe_game_world_step`, the emitters' and the water's runs by the
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
- `shortcuts.c` — the one read of those flags: the three Ctrl commands, Delete, an asset's under the
  Assets panel's keyboard, Ctrl+D, R, F and F2, the rest a step is recorded at with Ctrl+Z and Ctrl+Y
  on it, and Escape's raw and free edges.
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
- `game_tree.h` — what Play and Ship write and run: `<project>/Build/game/`'s six files, the
  `.gitignore`, the argument lists that build the tree as the game, the library or a release, and
  install it into the shipped folder `Build/ship/<name>/`, and the paths it reads and writes.
- `game_tree.c` — the files compared before they are written, the name and engine path escaped for
  where they go, the game window's numbers in main.c, the world cooked into scene.c, every prefab
  cooked into prefabs.c, landscapes.c and materials.c written, the release kind and the shipped
  folder.
- `game_tree_find.h` — every file under Assets/ with a given ending, any case, hidden entries
  skipped, in byte order of path, walked with an explicit stack of bounded depth.
- `game_tree_find.c` — the folder check, the stack walk, the case-blind suffix match and the sort.
- `game_tree_landscapes.c` — landscapes.c's text: every `.landscape` read and cooked in a rewound
  scratch, then voe_game_landscapes_cooked naming each by its `Assets/` path, size and cells.
- `game_tree_materials.c` — materials.c's text: every `.material` read and parsed in a rewound
  scratch, then voe_game_materials_cooked with each one's `Assets/` path and values, floats in hex.
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
  panel shown from the build log, the armed command that makes Close, New, Open and Back refuse
  once over unsaved work, and the `saved` flag a written Save leaves for main.c.
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
- `landscape_panel.h` — the Landscape panel: a title row with the file's name and its ×, and the
  size and cells as number boxes written at once, never unsaved or undone, as an anchored panel over the dock,
  carrying out nothing.
- `landscape_panel.c` — the panel's one frame of `ui` calls and the read of its controls afterwards,
  a drag handed back once on release, each number rounded and clamped to the file's range.
- `preferences.h` — Preferences: one row per theme with its name and a Choose button, the one in
  force marked, three sliders for that theme's contrast, separation and text size with a Reset
  button, and Close, as an anchored panel over the dock.
- `preferences.c` — the panel's one frame of `ui` calls and the read of its
  buttons and sliders afterwards.
- `errors.h` — the Errors panel a failed build shows: the last 48 lines of `Build/build.log`, each
  cut at 160 bytes, in a scroll area under a title row with its ×, as an anchored panel over the dock.
- `errors.c` — the log read back from its end to its last lines, the panel's one frame of `ui`
  calls, and the read of its × afterwards.
- `frame_breakdown.h` — the Frame panel: each GPU pass by name with its milliseconds and the total,
  copied from render's lagging timings four times a second, under a title row with its ×, anchored
  at a point.
- `frame_breakdown.c` — the quarter-second copy keeping the last on no measurement, the panel's one
  frame of `ui` calls, and the read of its × afterwards.
- `browser.h` — the editor's own file browser: a folder listing as an anchored panel over the dock,
  its own arena, a start beside a given folder with its row chosen, in SAVE mode a name row with a
  focused `ui` field and Make folder, and in IMPORT mode `.glb`, `.png`, `.jpg` and `.jpeg` files a
  press imports.
- `browser.c` — the browser's listing, its one frame of `ui` calls, and the read
  of its buttons and rows afterwards.
- `dock.h` — the tree of four panels, Scene over Assets on the left, whose splits hold a side
  panel's length or the views' share, the closable panels with a closed leaf not laid out, every
  seam's place (the reached one lit), whether a point is over a panel, and the walk.
- `dock.c` — the tree: the default one, the arrangement, held lengths and the views' share clamped
  and set, which views it shows and whether a point is over a panel.
- `dock_walk.c` — the walk to one frame of `ui` calls with its seams, each closable leaf's header of
  name and ×, each leaf a panel and scroll area handed to its panel's draw with the palette for the Scene list's drag marks, the Inspector's
  call with the open material, and each scene view's camera preview.
- `scene_list.h` — the Scene list: heading, Add entity and one row per authored entity as a tree, a
  row with children folding by its identity's saved flag, a copy marked with its prefab's file; a
  drag parents, unparents or, onto the Assets panel, makes a prefab; a part never drags.
- `scene_list.c` — the list's one frame of `ui` calls, walking the parent tree with a capped
  stack, rows tightly padded in one gapless column, each in a rim wrapper, a copy's file name,
  the drag's ghost refused or not, and the held row's drop.
- `drag_ghost.h` — the ghost every editor drag shows beside the pointer: a raised panel of the
  dragged thing's name, dimmed with "Can't drop here" when a release would drop nothing.
- `drag_ghost.c` — the dim pushed when refused, the anchored panel, the name and the second line.
- `assets_panel.h` — the Assets panel: `<project>/Assets/` as rows in its own arena, never above
  it, asking for a folder, landscape or material create, rename, delete or move named in place, a row held for a drag, a prefab,
  landscape or material to open, and Import.
- `assets_panel.c` — the listings with the selection kept, the rows filled in two passes, the
  panel's one frame of `ui` calls with the naming's field, the read of rows, keyboard, request, a
  fired prefab's, landscape's or material's path unless dragged, Up and Import afterwards, and the import.
- `assets_menu.h` — the right button's menu over the Assets panel: Rename, Duplicate and Delete
  over a row, Create with its Folder, Landscape and Material submenu over the empty part, anchored
  at the pointer.
- `assets_menu.c` — the menu's one frame of `ui` calls, the read of its rows with its own closing,
  and where it and the submenu go next frame.
- `assets_ask.h` — the question Delete asks about the Assets panel's selected row: its name, up to
  four users and "and N more" walked once on opening, Delete and Cancel, as an anchored panel.
- `assets_ask.c` — the open with its one walk and lines, the panel's one frame of `ui` calls, and
  the read of its buttons and a press outside afterwards.
- `assets_drag.h` — a held row from the Assets panel: released over a folder row or Up it moves
  there; a model, prefab or picture over a scene view places a new thing or a copy where the ray
  lands, over the Inspector swaps the selected thing's model or emitter's texture; elsewhere nothing.
- `assets_drag.c` — the drag started from the panel's held row, the one outcome at a pointer for
  the release and the ghost, the move's target, the drop point, the Inspector's rectangle, the
  texture swap, the open material following a move, and the one undo step.
- `assets_walk.h` — the project's `.scene`, `.prefab` and `.material` texts walked: who names a path before a
  Delete, and a rename's paths checked before the move and written after it, 128 bytes the room.
- `assets_walk.c` — the one walk as a stack of listings, a file read and rewound past at a time,
  the match, and the follow with its room check and write.
- `assets_manage.h` — the Assets panel's file commands: a folder, flat landscape or default material
  made, a rename or move followed on disk, in the open scene and in the model store, a duplicate,
  the trash, each refused with a notice and nothing changed, the materials read again after.
- `assets_manage.c` — the shared checks, a made file's name and write, the landscape's flat file,
  the material's defaults, the move's check, move,
  store rename, write and memory with the undo line forgotten, the next free copy name, and the
  trash's refusals worded.
- `resize.h` — the borders a person drags to size the panels: each side panel's seam, the Assets
  panel's, the views' and the top bar's lower edge, hit-tested before `ui`, the pointer's shape, the
  reached seam for the walk to light, and a double-click setting one size back (ADR-0226).
- `resize.c` — the hover, press, drag and release against the tree laid out below the bar, the
  double-click timed on the caller's clock.
- `panels.h` — the editor's panels as the person left them: the default tree started with the
  settings file read over it, the sizes remembered back into it when a drag ends, the toggle that
  opens or closes a dock panel, Project, Errors or the frame breakdown, and whether one is open.
- `panels.c` — the default tree's sizes filled in, read over and set back, and the same fields
  written through `settings.h`.
- `interface.h` — the screen-filling surface, made in the theme it is handed: pixels per millimetre
  from the window's height, the top bar above each root's dock tree, the browser, Preferences,
  Project, Errors, the frame breakdown or the colour picker over it, and one draw command per root.
- `interface.c` — one `ui` frame per root: the dock walk, the draws over the dock, and the one read
  of its clicks as calls, in order, into the two below.
- `interface_assets.h` — the Assets panel's share of that read: its clicks, naming request, menu
  row, Delete question and a fired prefab, landscape or material opened.
- `interface_assets.c` — the request and menu row carried out, a made landscape's path, the
  Landscape panel opened, a material opened in the Inspector, the question answered and opened,
  and the open material following each rename and trash.
- `interface_read.h` — the rest of that read: the Inspector, colour picker and Scene list first,
  then the ×, Panels list, top bar, browser, Preferences, Project, Landscape, Errors and breakdown.
- `interface_read.c` — the Inspector's reads, an edited material handed to its row and the store,
  the picker's submit, the drop's prefab, the reveal,
  the panels' toggles, the bar's or browser's commands, the covers' reads and Preferences'.
- `inspector.h` — what the selected entity is made of, the controls that change it, and the struct
  one frame of them is recorded in; it holds the shape of the open dropdown, because this panel
  draws that list and reads what was picked from it; a prefab's part is shown, never edited.
- `inspector.c` — the Duplicate and Delete row, the walk over described component types, each
  section's heading, Remove and "Needs" line, a row per field, each control's record, Add component
  and its menu, the open list, a part shown read-only, the Sculpt section on a landscape, and an
  open material's section in place of it all.
- `sculpt.h` — the brush a person holds: chosen or not, its kind, radius, strength and softness
  with their ranges, never saved or undone, whether a thing wears a landscape, and the held drag
  that sculpts it as one undo step.
- `sculpt.c` — the defaults set, the brush put down, the `.landscape` ending matched on a thing
  not a prefab's part, the ground under the pointer, the press's copy, the stamps along the drag
  and the release's stroke.
- `inspector_sculpt.h` — the Inspector's Sculpt section: Raise, Lower, Smooth and Flatten with the
  chosen one lit, and Radius, Strength and Softness sliders, read after the frame.
- `inspector_sculpt.c` — the section's one frame of `ui` calls, its nodes forgotten each frame, and
  the read: a fired button chooses its kind or none, each slider's value taken.
- `inspector_material.h` — the Inspector's material section: Lit and Unlit, the Colour swatch,
  Roughness and Metal sliders, Repeat clamped 0.01–1000, and the three map rows with ×.
- `inspector_material.c` — the section's one frame of `ui` calls, its nodes forgotten each frame,
  and the read into the shown copy only.
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
- `view.h` — a scene view: its orbit, target, remembered casters and input, the fly's speed the
  wheel sets and a middle press resets, the glide to a focus,
  the view under the pointer, the colours it is drawn with, and the preview of what the world's
  camera sees.
- `view.c` — the views' orbit, which owns the eye, the drag's and fly's rates, the glide, their
  targets, the focus set to the world camera's position, the world's first light row and the
  selection's outline colour, dimmed for a gizmo handle at rest and fainter still for a blocker's box.
- `view_passes.h` — what a frame draws into the views: a pass per shown view after its shadow
  passes, lit by every light, with the world, its models, the selection's outline, collider and
  gizmo, every blocker's box, the markers, the brush circle, and the device capacities.
- `view_passes.c` — the preview's pass, then each shown view's shadow passes and pass, lit by every
  light and kept out of the blockers, every place marked and blocker lined, the brush's rings with
  the gizmo hidden while brushing, stopping at the first refused pass.
- `models.h` — the editor's one model store: loaded from the project folder, re-read once a second,
  emptied on a different project, a broken file in the notice, a splash wait's progress passed on,
  its landscapes drawn, saved, reverted, put back by a stroke and reshaped, and its materials'
  table with an edited row set live or reloaded.
- `materials.h` — the project's `.material` files under Assets/ as game/materials.h's table, read on
  a project and after an Assets command, found by path to edit in place.
- `materials.c` — the walk, each file read and parsed into the next row or left out on stderr, the
  table and the scan.
- `models.c` — the store made, emptied on a new folder, filled and re-read through game/models.h
  with a failure's notice, cleared through the device and destroyed; landscapes' heights written
  in the frame, written on Save, read again on a New or Open, a size and cells written and re-read,
  and an edited material set or reloaded.
- `scene.h` — the project's world and selection, and the Scene panel and Inspector state built on
  them: rows, folds, drag, picker, gizmo mode, the reveal of a selection made elsewhere and the
  material open in the Inspector.
- `scene.c` — the selection closing the open material, its follow of a move, Delete and Duplicate, the gizmo's switch, the colour picker and
  dropdown targets, the Scene panel's rows and folds, asked after the frame has ended, and the
  reveal's unfold and scroll.
- `pick.h` — a left click in a scene view selects what is under the pointer, every placed thing on
  its mesh or its marker, a marker before a mesh, and a click on nothing clears the selection; the
  ray and what it meets are `3d`'s (ADR-0202, 0354).
- `pick.c` — the press edge, the view the pointer is over, the ray through that
  view's picture, and the selection set from whatever it met.
- `gizmo.h` — what the primary button does to the selected entity's gizmo, arrows or rings: the
  handle under the pointer, a press that grabs one, and the drag that submits its new position or
  rotation; a part has none.
- `gizmo.c` — the hover, the grab and the move or turn, each against a gizmo built from the view's
  own camera, measured from the press in world space and submitted as a whole transform, a child's
  written back relative to its parent.
- `undo.h` — the line of whole-scene texts that Ctrl+Z and Ctrl+Y step through (ADR-0204), a state
  carrying a sculpting stroke beside its text, with the level's line set aside while a prefab is open.
- `undo.c` — the states pushed once, the two lines swapped, a settled edit's compare and a reveal's
  amend, a stroke pushed, what could have been redone thrown away with its strokes, and a step
  writing the stroke it passes and re-finding the selection by authored id.
- `strokes.h` — one sculpting stroke: a landscape's path, the rectangle touched and its heights
  before and after, its own memory beside the undo line's text, and why.
- `strokes.c` — the copies made and freed, and the heights written either way through the store.
- `entities.h` — Add entity, a dropped model's thing or prefab's copy, duplicating and deleting
  entities with their trees, and giving or taking components, all through the world's structural
  queue. Its header says the id and name rules.
- `entities.c` — the identity room check before a make, the new id and name, the queued rows, a
  tree's destroys, and the destroy that undoes a half-made entity.
