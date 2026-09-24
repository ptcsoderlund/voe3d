# Needs decision — how the editor builds and runs a game

## Question
Play is the first game build (0187, ADR-0052), and nothing of it exists: no cook, no game program,
no way to start a process. Before a card can be cut, someone with reach over milestones 3 and 6
must say where a game's build tree lives, how it finds the engine, and which folders hold the
cook and the game's own loop. Each answer binds milestone 3 (the project's C compiled into the
same build) and milestone 6 (the same build in release, shipped), and some add folders and edges
to `cmake/voe.cmake`, which rule 2 makes a decision.

The parts that follow from any answer, and that the cards will carry once it is made:
- `platform/include/platform/process.h`: start a program with arguments, ask whether it still
  runs and how it ended, end it. Linux and Windows backends.
- A cook: a world written as one C translation unit of entity and component tables (ADR-0144),
  through the field descriptions, so it runs in the editor's build only.
- A game program: opens 1280×720 (0234), builds the cooked world, draws each frame through the
  scene's one camera (0218, 0222) filling the window, stops when its window closes.
- `editor`: a Play/Stop button in the top bar that cooks the world in memory without touching
  the project's saved state (0188), runs the build, starts the program, and polls it each frame.

## Options

**A. A game build tree per project, outside this repository, naming the engine by its source
path.** The editor writes `<project>/.voe3d/game/CMakeLists.txt` (or under `<settings>/voe3d/`),
which `add_subdirectory`s each engine folder the game links from the engine source path compiled
into the editor, adds the cooked `scene.c` and a new engine folder `game` (the loop: window,
device, world, draw through the camera; a program library a game's `main` calls), and runs
`cmake` then `ninja` in it. A new folder `cook` (row: `scene ecs math base`, authoring-time like
`authoring`) turns a world into C; `editor`'s row gains `cook` and `game` stays below `editor`.
Descriptions are off in that tree, as ADR-0145 point 3 says. Cost: the first Play in a project
compiles the engine libraries once (tens of seconds); every Play after is a one-file rebuild and
meets ADR-0055. Milestone 3 adds the project's `.c` files to the same generated list; milestone 6
is the same tree in release.

**B. The game is a target in the editor's own build tree.** A `game` folder in this repository
builds `voe_game` from a cooked source path handed in as a cache variable, and the editor runs
`cmake --build build/debug --target voe_game`. Fastest by far: the engine is already built.
Cost: a game compiled with descriptions on, against ADR-0145 point 3; Play works only from a
checkout with a configured build tree, which a shipped editor does not have; milestone 3 puts a
project's sources into the engine's tree.

**C. No build yet: a player program reads the scene text.** A `play` program links `authoring`,
the editor writes the scene text to a temporary file and starts it. Smallest, nearly all reuse.
Cost: contradicts 0187 and ADR-0151 (a game does not link `authoring`) and is thrown away in
milestone 3.

## Recommendation
**A**, with the build tree under the project (`<project>/.voe3d/game/`, ignored by the project's
git), the engine source path compiled into the editor as a definition, and the two new folders
`cook` and `game` with the rows above. It is the only option that is already the shape
milestones 3 and 6 need, and it keeps ADR-0145 and ADR-0151 whole. The first-Play cost should be
accepted in the decision, or answered by one shared engine build per editor under
`<settings>/voe3d/` that every project's tree links, so that no Play but the very first pays it.
