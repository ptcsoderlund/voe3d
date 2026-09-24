# 0242 — A project's code is `Code/`, built as a library the editor loads into a new world
date: 2026-09-24
by: planner

## Decision
For 025, filling in what 0239, 0240 and 0241 left to the planner:

1. **The code is `<project>/Code/`**: every `*.c` there is compiled, and `Code/` is on the include
   path. A component's struct and its key have one name (`player`, key symbol `player_key`,
   `const voe_ecs_key player_key = {"player"}`), because the cook names both from the key.
2. **Two entry points, declared in `game/project.h` and defined by the project**:
   `voe_game_project_register(world)` and `voe_game_project_systems_run(step)`, the step holding
   world, window and seconds. A project with no `.c` gets empty ones from a file `cmake/game.cmake`
   writes into its binary folder. The game's `run.c` registers them after the world and runs the
   systems before each `voe_game_frame`; the editor never runs them. Resolved at link time in the
   game and by one `dlsym` in the editor: the ADR-0008 seam, the only function pointer outside render.
3. **A project type is registered by `game`**: `voe_game_project_component` registers the table,
   a replace intent (entity at offset 0, row at the next `max_align_t` boundary) under the Nth of
   `game`'s own VOE_GAME_PROJECT_TYPES (32) intent keys, a default (zeros when none) and a menu path.
   A row is at most VOE_GAME_PROJECT_ROW (240) bytes, so its intent fits the Inspector's 256.
   `voe_game_project_replaces_apply` applies those intents whole, after the structural queue, in
   `voe_game_frame` and in the editor's world step: the owner of a project row is its project, and
   the one generic writer is `game`. Project systems write their own rows directly and other
   folders' through their intents. The world has room for 32 more types and intents.
4. **The library**: the editor configures the same `Build/game/` tree a second time, into
   `Build/editor/`, with `-DVOE_GAME_LIBRARY=ON`; there `game.cmake` builds only the SHARED target
   `project` from the code, with descriptions on and `$<COMPILE_ONLY:voe::game>`, so no engine code
   is in it. The editor is linked with its exports and every folder but `game` whole-archive, so the
   library's undefined engine symbols bind to the editor's own. Windows: library mode is refused by
   `game.cmake` with a message, for now; the game is unchanged there.
5. **A load copies the built library** to `Build/editor/loaded/project-<n>.<ext>`, n counting loads
   in this run, and opens the copy: a loaded file is locked on Windows, and glibc hands back the old
   handle for a path already open. A build whose library bytes equal the loaded one's loads nothing.
6. **A swap is a new world**: the scene is written to text, kept sections included; a new world in an
   arena of its own gets the engine's types, then the library's; the text is read into it; then the
   old world's arena is destroyed and the old library closed, in that order. The unsaved flag and the
   undo line are untouched and the selection is re-found by authored id. A type gone is a kept section
   (0241). A failed build or read keeps the old world and library.
7. **Refresh**: a top-bar button, "Refreshing" while it runs; untitled refuses. Opening a project with
   code starts one. Play refreshes first when the project has code, reading "Building" through both
   builds; with no code and nothing loaded it plays as in 024. The session polls at the top of each
   frame, before the structural queue, and swaps there. A capture waits for a running refresh.
8. **Errors (0240)**: every build step's output goes to `Build/build.log`, emptied when a refresh or
   Play starts (`voe_platform_process_start` gains an output path; NULL is shared). A failed step keeps
   the stderr line, sets the notice, and shows an Errors panel over the dock, as Preferences is: the
   log's last 48 lines in a scroll area, and Close. A failed load names the loader's reason.
9. **The Inspector**: a heading drops a leading `voe_<folder>_` and capitalises every `_` word, so
   `keyboard_input` is "Keyboard Input"; an ENTITY field is a dropdown of None and the authored
   entities by name.
10. **The example project is `game/example/`**, beside the entry points it shows; it is data plus
    `Code/`, and its `.gitignore` keeps `Build/` out.

## Reasoning
One tree built twice keeps one list of sources for game and editor. A whole new world is the only
swap ecs allows (no unregister, no destroy) and reuses the text round trip undo already trusts; kept
sections already carry a vanished type. A generic replace in `game` spares every project an intent
per component. Rejected: `RTLD_LAZY` without exported symbols (Windows resolves at load anyway), a
project-wide plugin registry (0241), systems in the editor (0241), `examples/` at the root (a repo
layout, not this feature's).

## Replaces
Nothing.
