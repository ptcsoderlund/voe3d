# src

`editor`'s implementation. There is no public header: `main.c` is the program,
and every other file is a `.h` saying what it owns and why beside the `.c` that
carries it out.

- `main.c` — opens the project, window and device, starts and loads scenes on a worker behind a
  live splash, and runs the loop until a close goes ahead or the picture is written.
- `loading.h` — the start's, a New or Open load's, an import's prepare and the cook before Play and
  Ship works for a splash wait: shaders, shapes and models, the scene and its models, an imported
  model's levels file or every stale cooked copy made on the device, and what the worker owns.
- `loading.c` — the four works, each step after a stop check, in the worker's own scratch, a
  refused prepare's notice from the kept error.
- `waits.h` — the loop's splash waits between frames: an import's or Make levels' prepare then its
  reload, a swaps-only reload, the cook before Play and Ship, and New or Open's load, false where a
  wait was.
- `waits.c` — the due blocks moved from main.c, each context filled and waited on, and the cook's
  `.glb`s found and stale asked before its wait.
- `splash.h` — the engine's splashscreen.png, never a project's, read from the engine source at run
  time so a missing copy shows the plain screen, kept by the caller until the device closes.
- `splash.c` — the path joined from toolchain.h's engine folder and read with app/picture.h.
- `frame_commands.h` — the loop's keyboard commands: the shortcuts read against main.c's guards,
  the acts on them, Escape's order, the undo step taken next frame, and the acts after the draw.
- `frame_commands.c` — the loop's three per-frame stretches: the history step settling an open
  material's edit, the keyboard's read with its acts, and the acts that wait for the interface to
  have drawn.
- `frame_pointer.h` — the loop's pointer and view reads in their order: the fly, the shortcuts, a
  floating box's edges, a header's drag then the borders, the middle drag, the brush, the gizmo,
  the Assets drag and the pick, none starting under a floating box, and what main.c reads after.
- `frame_pointer.c` — the fly with its lock edge, a box's edges, a header's drag and the borders
  with their clears and remember, and the brush then the three left-press readers under one shared
  block a stroke joins, each kept from starting under a floating box.
- `float_edges.h` — a floating panel's box resized by its edges, two at a corner, at least
  30 × 20 mm inside the dock area, topmost box first, the press raising it; not the seams' file.
- `float_edges.c` — the topmost box's edge under the pointer, the raise, and the drag writing the
  box from the area's top left.
- `panel_drag.h` — a panel's header dragged past 1 mm to dock or float it at the drop target, a
  double-click on a floating one docking it back, asked before the borders by last frame's headers.
- `panel_drag.c` — the header under a press, floating boxes topmost first, the raise, the
  double-click, the drag's target each frame and the move on release.
- `frame_selection.h` — F: the view under the pointer glides to the selection's size from `3d`, or
  to a sizeless thing's place at a fixed 3 m, nothing saved or undone.
- `frame_selection.c` — the selection and view checked, the bounds or world position, the distance
  and the glide.
- `startup.h` — which project the editor opens on: the argued folder, the remembered one or
  untitled, written back as the last project unless capturing, and the descriptions line.
- `startup.c` — the three tried in order, what each failure says, and the last-project write.
- `world_step.h` — the world's step once a frame, the transforms remembered first for the bounce's
  stale spheres, `game`'s step, why it runs no move but runs emitters, water and the clouds' and
  fog's drift by the frame's seconds, and why placed copies are expanded after it.
- `world_step.c` — the remember, the one call to `voe_game_world_step`, the emitters', the water's,
  the clouds' and the fog's runs by the seconds, the point lights' replaces with none, then the
  expansion.
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
- `settings.h` — the panel sizes, the views' share, the dock panels' open flags and the snap keys
  `snap`, `snap_move`, `snap_rotate`, `snap_scale` a person left, remembered at
  `<settings>/voe3d/editor_settings`, one `<key> <value>` line each (ADR-0226, 0363, 0482), its
  `fold` lines folds.h's.
- `settings.c` — that file read line by line as a key and a number in range, the panel and snap
  keys alike, and written back with every other key's line kept, by making the two folders above
  it as needed.
- `settings_layout.h` — the dock tree as one line per node and each moving panel's floating box as
  one line in `editor_settings`, read back at start unchecked (0491 point 7).
- `settings_layout.c` — each line's words taken by a cursor, a node or float kept only when all
  parse, and the root, nodes and floats written through settings.h's keep-other-keys write.
- `folds.h` — the Inspector sections and foliage kinds a person folded, one `fold <name>` line each
  in `editor_settings` by section word or landscape and kind index (0502, 0503), written when one
  flips, never undone, and this frame's fold buttons.
- `folds.c` — the names kept in order, each fold button's name in the frame's arena, a fired one
  flipping its name, and the `fold` lines read and written through settings.h's shared calls.
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
  base/report.h's first kept error, the file put before it only when the line does not begin with it.
- `code.h` — a project's library loaded from a copy under `Build/editor/loaded/`, its one entry
  point resolved (ADR-0008), whether a build equals it, and why it closes after its worlds.
- `code.c` — the folders made, the copy read and written, the open and the lookup, and the compare.
- `game_tree.h` — what Play and Ship write and run: `<project>/Build/game/`'s six files, the
  `.gitignore`, the argument lists that build the tree as the game, the library or a release, and
  install it into the shipped folder `Build/ship/<name>/`, and the paths it reads and writes.
- `game_tree.c` — the game tree's files written only where their bytes changed, text escaped for
  where it goes, the world and every prefab cooked in, and the argument lists built.
- `game_tree_find.h` — every file under Assets/ with a given ending, any case, hidden entries
  skipped, in byte order of path, walked with an explicit stack of bounded depth.
- `game_tree_find.c` — the folder check, the stack walk, the case-blind suffix match and the sort.
- `game_tree_landscapes.c` — landscapes.c's text: every `.landscape` read and cooked in a rewound
  scratch, then voe_game_landscapes_cooked naming each by its `Assets/` path, size, cells, layers,
  paint, foliage kinds, copies, kinds' looks with their blend and each layer's growth of each kind.
- `game_tree_materials.c` — materials.c's text: every `.material` and `.parts` read and parsed in a
  rewound scratch, then voe_game_materials_cooked with each one's `Assets/` path and values, floats
  in hex, and voe_game_parts_cooked with each `.glb`'s path, its material paths and its levels.
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
- `session.h` — the project being worked on, the commands a person asks of it, Play and Ship
  waiting a frame for the cook before them, and the questions it holds: Delete, levels and Import
  textures.
- `session.c` — the refuse-once rule, the eight commands with Play and Ship refreshing first and
  cooking before they build, one build at a time and refused while a prefab is open, a prefab
  opened, a built library swapped in once a frame, and what a browser action does to the session.
- `topbar.h` — the bar across the top: Back while a prefab is open, the project's commands,
  Project, Preferences, Panels and its list, the gizmo's mode (Move, Rotate or Scale), Snap and its
  three steps, the name and unsaved mark, then the notice, as tall as its content or as made.
- `topbar.c` — the bar's one frame of `ui` calls, a panel holding one row, and the read of its
  buttons, Back, Play, Refresh, Ship and Project among them, and of Snap and its step boxes clamped
  to range, afterwards.
- `panels_menu.h` — the list Panels opens: one row per closable panel, an open one ticked with √ in
  a column of its own, as an anchored panel below the button, closed by its caller.
- `panels_menu.c` — the list's one frame of `ui` calls and the read of its rows, and whether the
  pointer is on it, afterwards.
- `project_panel.h` — the Project panel's contents in a dock panel: the game window's width and
  height as number boxes and a Windowed / Fullscreen choice, carrying out nothing.
- `project_panel.c` — the contents' one frame of `ui` calls and the read of its controls
  afterwards, a number rounded and clamped to the project file's range.
- `landscape_panel.h` — the Landscape panel's contents in a dock panel: the file's name, and the
  size and cells as number boxes written at once, never unsaved or undone, carrying out nothing.
- `landscape_panel.c` — the contents' one frame of `ui` calls and the read of its controls
  afterwards, a drag handed back once on release, each number rounded and clamped to the file's
  range.
- `preferences.h` — Preferences: one row per theme with its name and a Choose button, the one in
  force marked, three sliders for that theme's contrast, separation and text size with a Reset
  button, and Close, as an anchored panel over the dock.
- `preferences.c` — the panel's one frame of `ui` calls and the read of its
  buttons and sliders afterwards.
- `errors.h` — the Errors panel a failed build shows: the last 48 lines of `Build/build.log`, each
  cut at 160 bytes, drawn as a dock panel's contents in its scroll area.
- `errors.c` — the log read back from its end to its last lines, and the contents' one frame of
  `ui` calls.
- `frame_breakdown.h` — the Frame panel: each GPU pass's milliseconds, the total, triangles, draws,
  the textures' MB and each picture's counts, copied four times a second, drawn as a dock panel's
  contents.
- `frame_breakdown.c` — the quarter-second copy of the pictures' counts, the textures' memory and
  the triangle and draw counts, keeping the last times on no measurement, and the contents' one
  frame of `ui` calls.
- `frame_hitches.h` — the editor's hitch watch: its own span's CPU stretches, the pace's wait, the
  shadows' and bounce's parts and foliage gathers timed, and an `editor: hitch:` line on stderr for
  each frame stalled on the CPU or the card, saying whether a light bounced (0458, 0461, 0463).
- `frame_hitches.c` — the stretches read off the platform clock, the wait and the shadows' sums
  added, then app/hitches.h asked about the frame since `_begin` and the card's lagging measurement.
- `browser.h` — the editor's own file browser: a folder listing as an anchored panel over the dock
  to open a project, save into a folder or import a model or picture, in its own arena, a showing
  able to start beside a given folder.
- `browser.c` — the browser's listing, its one frame of `ui` calls, and the read
  of its buttons and rows afterwards.
- `dock.h` — the panel tree, its splits holding a side panel's length or the views' share, the
  closable panels (a closed or floating leaf not laid), floating boxes and the one under a point,
  last frame's headers, every seam's place, the walk, a drop, move and dock-back, a tree's check.
- `dock.c` — the tree: the default one, the arrangement, held lengths and the views' share clamped
  and set, which views it shows, the floating box under a point, whether a point is over a panel,
  and the check's stack walk.
- `dock_walk.c` — the walk to one frame of `ui` calls: its seams, each closable's header of name
  and ×, each leaf a panel and scroll area handed to its panel's draw, then each floating panel as
  an anchored box over the tree, a dragged panel's drop outline, and each view's camera preview.
- `dock_move.c` — a panel's rectangle, the drop target's first match with its outline, the leaf
  taken out, the tree compacted and a split put in at the target, a float raised, and the defaults.
- `scene_list.h` — the Scene list: heading, Add entity and one row per authored entity as a tree, a
  row with children folding by its identity's saved flag, a copy marked with its prefab's file; a
  drag parents, unparents or, onto the Assets panel, makes a prefab; a part never drags.
- `scene_list.c` — the list's one frame of `ui` calls, walking the parent tree with a capped
  stack, rows tightly padded in one gapless column, each in a rim wrapper, an off row greyed, a
  copy's file name, the drag's ghost refused or not, and the held row's drop.
- `drag_ghost.h` — the ghost every editor drag shows beside the pointer: a raised panel of the
  dragged thing's name, dimmed with "Can't drop here" when a release would drop nothing.
- `drag_ghost.c` — the dim pushed when refused, the anchored panel, the name and the second line.
- `assets_panel.h` — the Assets panel: `<project>/Assets/` as rows in its own arena, never above
  it, asking for a folder, landscape or material create, rename, delete or move named in place, a row held for a drag, a prefab,
  landscape or material to open, and Import.
- `assets_panel.c` — the listings with the selection kept, the rows filled in two passes, the
  panel's one frame of `ui` calls with the naming's field, the read of rows, keyboard, request, a
  fired row's path, Up and Import afterwards, and the import's copy.
- `model_import.h` — what Import writes beside a `.glb`: one `<stem>-<name>.material` per glTF
  material and `<stem>.parts` naming them, both overwritten every import, the levels kept (0403,
  0420 point 3); and Import textures: its pictures as PNGs within a largest size, the four maps set.
- `model_import.c` — the file read and parsed, its pictures decoded only for Import textures, each
  name cleaned and made unique, every path checked before the first write, then the materials and
  the parts, or the PNGs shrunk to the largest size by use and the dressed materials, written.
- `assets_menu.h` — the right button's menu over the Assets panel: Rename, Duplicate and Delete
  over a row and Import textures and Make LODs over a `.glb`'s, Create with its Folder, Landscape and Material
  submenu over the empty part, anchored at the pointer.
- `assets_menu.c` — the menu's one frame of `ui` calls, the read of its rows with its own closing,
  and where it and the submenu go next frame.
- `assets_ask.h` — the question Delete asks about the Assets panel's selected row: its name, up to
  four users and "and N more" walked once on opening, Delete and Cancel, as an anchored panel.
- `assets_ask.c` — the open with its one walk and lines, the panel's one frame of `ui` calls, and
  the read of its buttons and a press outside afterwards.
- `levels_ask.h` — the anchored panel a model's LODs ask: its count and a row per LOD
  with share, swap, Swap distance in metres and triangles, with Make and Cancel; Make saves into
  the `.parts` and says whether a reload is enough.
- `levels_ask.c` — the open with its quiet `.parts` read and triangle count, the panel's one frame
  of `ui` calls with its column headers, LOD rows and swap line, the read of its buttons, boxes and
  a press outside, and the save into the `.parts`.
- `textures_ask.h` — the anchored panel Import textures asks: a largest size of 512, 1024, 2048
  or 4096, 2048 on every opening, with Import and Cancel; the caller carries out the answer.
- `textures_ask.c` — the open with its title, the panel's one frame of `ui` calls with the chosen
  size lit, and the read of its buttons and a press outside.
- `assets_drag.h` — a held row from the Assets panel and what its release does: a move over a
  folder row or Up, a new thing or copy placed in a scene view, or over the Inspector a swap, a
  shape's material, a filled map, a landscape layer or a foliage kind, an added one open, else nothing.
- `assets_drag.c` — the drag started from the panel's held row, the one outcome at a pointer for
  the release and the ghost, the move's target, the drop point, the Inspector's rows, layer and
  kind rows among them, and each swap with its undo step.
- `assets_walk.h` — the project's `.scene`, `.prefab`, `.material`, `.landscape` and `.parts` texts
  walked: who names a path before a Delete, and a rename's paths checked before the move and written
  after it, 128 bytes the room.
- `assets_walk.c` — the one walk as a stack of listings, a file read and rewound past at a time,
  the match, and the follow with its room check and write.
- `assets_manage.h` — the Assets panel's file commands: a folder, flat landscape or default material
  made, a rename or move followed on disk, in the open scene and in the model store, a duplicate and
  the trash, each refused with a notice and nothing changed.
- `assets_manage.c` — the shared checks, a made file's name and write, the landscape's flat file,
  the material's defaults, a `.glb`'s `.parts` found, the move's check, move, store rename, write
  and memory with the undo line forgotten, the next free copy name, and the trash's refusals worded.
- `resize.h` — the borders a person drags to size the panels: each side panel's seam, the Assets
  panel's, the views' and the top bar's lower edge, hit-tested before `ui`, the pointer's shape, the
  reached seam for the walk to light, and a double-click setting one size back (ADR-0226).
- `resize.c` — the hover, press, drag and release against the tree laid out below the bar, the
  double-click timed on the caller's clock.
- `panels.h` — the editor's panels as the person left them: the kept tree if it passes its check,
  else the default with the size keys read over it, sizes and layout remembered when a drag ends,
  the toggle that opens or closes a panel, and whether one is open.
- `panels.c` — the kept layout read and checked, else the default tree's sizes filled in, read over
  and set back, and the same fields and the layout written through `settings.h` and
  `settings_layout.h`.
- `interface.h` — the screen-filling surface, made in the theme it is handed, its one draw
  command per root, and the frame's budget, the walk's four tool panels in it.
- `interface.c` — one `ui` frame per root: the dock walk, the draws over the dock, and the one read
  of its clicks as calls, in order, into the two below, the bar's snap remembered when it changed.
- `interface_assets.h` — the Assets panel's share of that read: its clicks, naming request, menu
  row, Delete, levels and Import textures questions and a fired prefab, landscape or material
  opened.
- `interface_assets.c` — the Assets panel's requests and menu rows carried out, Make LODs and
  Import textures opening their questions, and the Delete, levels and Import textures questions
  answered, Make's asking the prepare or, for swaps alone, a reload, Import's importing at its size.
- `interface_read.h` — the rest of that read: the Inspector, colour picker and Scene list first,
  then the ×, Panels list, top bar, browser, Preferences, Project and Landscape.
- `interface_read.c` — the Inspector's reads, an edited material handed to its row and the store,
  the picker's submit (a material's into the field its offset names), the drop's prefab, the reveal,
  the panels' toggles, the bar's or browser's commands, the covers' reads and Preferences'.
- `inspector.h` — what the selected entity is made of, the controls that change it, and the struct
  one frame of them is recorded in; it holds the shape of the open dropdown, because this panel
  draws that list and reads what was picked from it; a part is shown, never edited; sections fold.
- `inspector.c` — the Enabled row's button, the Duplicate and Delete row, each described
  component's folding section with its heading and Remove, then "Needs" line and fields, Add
  component, its menu and open list, a part read-only, a landscape's sections, or a material's.
- `inspector_rows.h` — one described field's row: its name and the control its kind gets, a list of
  strings a text field per string, a ranged real a slider with its figure, labels when it cannot be
  edited.
- `inspector_rows.c` — each kind's row and each control's record, a rotation's three angles, a
  ranged field's slider over its clamped value and figure, and a rank-2 CHAR's numbered rows each
  recording its string's offset, size, field and index.
- `sculpt.h` — the brush a person holds with its ranges, never saved or undone, its sections'
  nodes with a kind's blend controls, whether a thing wears a landscape, the held drag that
  sculpts, paints, scatters or erases as one undo step, a kind's look edit and a layer's grow edit.
- `sculpt.c` — the defaults, the brush put down, the `.landscape` ending matched, the look and
  grow edits set live and closed as strokes, the ground under the pointer, the press's copy, the stamps
  along the drag, and the release's heights, paint or foliage stroke.
- `inspector_sculpt.h` — the Inspector's Sculpt section, which folds: Raise, Lower, Smooth and
  Flatten with the chosen one lit, none while painting or brushing foliage, and the Strength slider,
  read after the frame.
- `inspector_sculpt.c` — the folding section's one frame of `ui` calls, its nodes forgotten each
  frame, and the read: a fired button chooses its kind or none, the slider's value taken.
- `inspector_brush.h` — the Inspector's Brush section, which folds, after Foliage while any brush is
  chosen: Radius (m) and Softness sliders every brush shares, read after the frame.
- `inspector_brush.c` — the folding section's one frame of `ui` calls, its nodes forgotten each
  frame, and the read: each drawn slider's value taken into the radius or softness.
- `inspector_layers.h` — the Inspector's Layers section, which folds, under Sculpt: a row per layer
  by its material's file name choosing it to paint, none lit while brushing foliage, the painted
  layer's grow rows ticking each kind with a Density, and the add row a dropped material lands on.
- `inspector_layers.c` — the worn loaded grid found, the folding section's one frame of `ui` calls,
  and the read: a tick or Density asks the layer's growth, a fired row chooses PAINT with its layer
  or none, each row's rectangle kept.
- `inspector_foliage.h` — the Inspector's Foliage section, which folds, under Layers: Scatter and
  Erase, a folding box per kind whose title toggles it picked, holding its Base size, Cast shadows,
  Blend with ground, Blend height and spreads, the add row a dropped `.glb` lands on, Density.
- `inspector_foliage.c` — the folding section and kind boxes' one frame of `ui` calls, its nodes
  forgotten each frame, and the read: a fired button chooses its kind or none, a fired title flips
  its pick, Density clamped, a kind's drawn slider or button asks its look with that field changed.
- `inspector_level.h` — the Inspector's LOD section, which folds, after Brush or Foliage: per view in
  use, the LOD it keeps of a loaded model, as its draws chose it last, else the distance's; of how
  many, made or own.
- `inspector_level.c` — the worn loaded model found, the folding LOD section's one frame of `ui`
  calls, each view's label formatted into its kept buffer.
- `inspector_material.h` — the Inspector's material section: Lit and Unlit, the Colour and Emission
  swatches, Strength clamped 0–1000, Roughness and Metal sliders (greyed and unread while ORMmap is
  set), Repeat clamped 0.01–1000, Cutout, One side and Both sides, and the four map rows with ×.
- `inspector_material.c` — the section's one frame of `ui` calls, its nodes forgotten each frame,
  and the read into the shown copy only, a swatch opening the picker on its field's offset.
- `add_menu.h` — the entries Add component offers one entity, a tree of groups and types built each
  frame from the types' registered menu paths (ADR-0217, 0221), and each level drawn as a list.
- `add_menu.c` — each offered type's path split on `/` and trimmed, its groups found or made under
  their parent, its entry appended in registration order, and one parent's children drawn as a list
  the way the open dropdown's is.
- `inspector_edit.h` — the calls that turn what the pointer did to the Inspector's controls into
  replace intents and scene.h's commands, and the rules for when each open list closes.
- `inspector_edit.c` — a dragged or typed number or a moved slider submitted as the component's
  replace intent, a rotation's edit as the difference about a world axis, a committed text field
  as the row's CHAR bytes, and the picker's colour and the open list's choice submitted the same
  way.
- `inspector_buttons.c` — the fired buttons, Enabled's switch among them, carried out through scene.h and entities.h, and the
  open dropdown and Add component's lists and submenus opened, closed and placed each frame; none
  on a prefab's part.
- `inspector_place.h` — the side-and-cap rule the open lists and submenus are placed by, and the
  rectangle test a press outside them is.
- `inspector_place.c` — a list fitted below, above or capped on the roomier side, under its button
  or beside its row, in the content column's space.
- `inspector_value.h` — what a field's bytes say: a kind and an offset in, a number, three shown
  angles or the one string a label is given out, and a ranged value's figure with its unit.
- `inspector_value.c` — a number read out of a field's bytes whatever its width, the Z-Y-X
  decomposition of a rotation, a type's heading from its key, a name's title, a field or one of a
  list's strings as one string, and how many
  boxes a kind is worth.
- `entity_field.h` — what an ENTITY field offers and says: None and the authored entities by name,
  and why names and only authored ones.
- `entity_field.c` — the identity table sorted by id into the choices, and a name or "None" as the
  label.
- `view.h` — a scene view: its orbit, target, remembered casters, kept LODs and input, the fly's speed the
  wheel sets and a middle press resets, the glide to a focus,
  the view under the pointer, its colours, the preview of what the world's camera sees, and each
  picture's counts.
- `view.c` — the views' orbit, which owns the eye, the drag's and fly's rates, the glide, their
  targets, the focus set to the world camera's position, the world's first light row and the
  selection's outline colour, dimmed for a gizmo handle at rest and fainter still for a blocker's box.
- `view_passes.h` — what a frame draws into the views: a pass per shown view after its shadow
  passes, lit by every light, sky, clouds and fog from its own eye, the world, models, foliage,
  outline, collider, gizmo, blockers' and fog volumes' boxes, markers, brush, counts, capacities.
- `view_passes.c` — the preview's pass, then each shown view's shadow passes and pass, each with
  its own LOD memory, foliage after the lights, kept out of the blockers, places marked, blockers
  and fog volumes lined, stopping at the first refused pass, shadows and gathers timed, counts kept.
- `models.h` — the editor's one model store: loaded from the project folder, re-read once a second
  or on asking, emptied on a different project, a broken file in the notice, where landscapes and
  materials are edited, models given parts, and maps compressed on the GPU between frames.
- `materials.h` — the project's `.material` files under Assets/ as game/materials.h's table, read on
  a project and after an Assets command, found by path to edit in place.
- `materials.c` — the walk, each file read and parsed into the next row or left out on stderr, the
  table and the scan.
- `parts.h` — the project's `.parts` files under Assets/ as game/materials.h's parts table, each row
  named by its `.glb`, read when the materials are.
- `parts.c` — the walk, each file read and parsed into the next row named by its `.glb` or left out
  on stderr, and the table.
- `models.c` — the editor's one model store, filled and watched between frames, its materials'
  maps watched too, and its landscapes with their foliage, kinds' looks, layers' growth and
  materials edited, its map queue stepped and the rows naming a written or changed map reloaded.
- `map_watch.h` — the maps the project's materials name, each one's stamp at the last look, a
  changed one reported once a second so its materials load again; the editor's, not `game`'s.
- `map_watch.c` — the look's marks, one stamp a map under the folder, a new path noted, a change
  reported while room is left, and the unnamed paths let go.
- `scene.h` — the project's world and selection, and the Scene panel and Inspector state built on
  them: rows, folds, drag, picker, the gizmo's mode of three, the reveal of a selection made
  elsewhere and the material open in the Inspector.
- `scene.c` — the selection and its targets, the open material's close, follow and refresh,
  Delete, Duplicate and the Inspector's switch, and the Scene panel's rows, asked after the frame
  has ended.
- `pick.h` — a left click in a scene view selects what is under the pointer, every placed thing on
  its mesh or its marker, a marker before a mesh, and a click on nothing clears the selection; the
  ray and what it meets are `3d`'s (ADR-0202, 0354).
- `pick.c` — the press edge, the view the pointer is over, the ray through that
  view's picture, and the selection set from whatever it met.
- `gizmo.h` — what the primary button does to the selected entity's gizmo, arrows, rings or
  scales: the handle under the pointer, a press that grabs one, and the drag that submits its new
  position, rotation or scale, the scales on the entity's own axes, snapped by Snap XOR Ctrl.
- `gizmo.c` — the hover, the grab and the move, turn or scale drag, each against a gizmo built
  from the view's own camera, measured from the press in world space, its world value snapped,
  and submitted as a whole transform, a child's written back relative to its parent.
- `undo.h` — the line of whole-scene texts that Ctrl+Z and Ctrl+Y step through (ADR-0204), a state
  carrying a sculpting stroke or material edit beside its text, with the level's line set aside
  while a prefab is open.
- `undo.c` — the states pushed once, the two lines swapped, a settled edit's compare and a reveal's
  amend, a stroke or material step pushed, what could have been redone thrown away with them, and a
  step applying the one it passes and re-finding the selection by authored id.
- `material_steps.h` — one material edit: a `.material`'s path and its values before and after, its
  own memory beside the undo line's text, and the file written at once.
- `material_steps.c` — the copy made and freed, the file's write, and the values put back either
  way into the table, file, store and open copy.
- `strokes.h` — one sculpting, paint, foliage, look or grow stroke: a landscape's path, the
  rectangle touched and its heights or weights before and after, the copies added or removed, a
  kind's look or a layer's growth before and after, its own memory beside the undo line's text.
- `strokes.c` — the copies made and freed, and the heights, paint, foliage, kind's look or layer's
  growth written either way through the store.
- `entities.h` — Add entity, a dropped model's thing or prefab's copy, duplicating and deleting
  entities with their trees, and giving or taking components, all through the world's structural
  queue. Its header says the id and name rules.
- `entities.c` — the identity room check before a make, the new id and name, the queued rows, a
  tree's destroys, and the destroy that undoes a half-made entity.
