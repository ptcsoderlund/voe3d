# 0237 — The cook is authoring's, the game loop is a `game` folder, Play is a child process
date: 2026-09-24
by: planner

## Decision
For 024, filling in what 0235 left to the planner:

1. **The cook is in `authoring`** (`scene_cook.h`). It turns a world into C source text: one
   function, named by the caller, that creates one entity per authored entity (ascending by id) and
   adds each described, non-runtime-only row with `voe_ecs_component_add` from a compound literal.
   Floats are hex literals, so the bytes are the editor's exactly; ENUM is an integer; ENTITY is the
   cooked entity with that authored id or the zero entity; kept sections are not cooked. The cooked
   function is a third creation exception to rule 3, beside a typed creation call and the scene
   reader: it adds rows to entities it has just made and edits none.
2. **A new library folder `game`**, after `app`, row `base math ecs scene platform render 3d app`.
   It owns the one list of component types a project's world registers (`world.h`, used by the editor
   too, so the editor gains the edge `game`), the header the cooked source includes (`scene.h`,
   declaring `voe_game_scene_build`, which the cooked `scene.c` defines, not this folder), one frame
   of the game (`frame.h`) and the program's whole run (`run.h`). The symbol is resolved at link time
   in the game tree; no function pointer (ADR-0135).
3. **The game tree**: the editor writes `<project>/Build/game/` (`CMakeLists.txt`, `main.c`,
   `scene.c`), each only when its bytes differ, and builds into `<project>/Build/debug/` with Ninja
   and `CMAKE_BUILD_TYPE=Debug`. The tree's `CMakeLists.txt` names the engine source and includes
   the engine's `cmake/game.cmake`, which holds the game build. The editor knows the engine source,
   CMake, the compiler, Ninja, pkg-config, slangc and wayland-scanner from a header its own build
   generates (`toolchain.h`); an empty value is a `-D` left out. A project without a `.gitignore` is
   given one listing `/Build/` and `/Cache/`; an existing one is never touched.
4. **`VOE_FOLDER_DATABASES`** (default ON) guards the copy of `compile_commands.json` into engine
   folders; the game tree turns it OFF, as it does `VOE_BASE_DESCRIPTIONS`.
5. **`platform/process.h`** starts a program from an argument list with the editor's stdout and
   stderr, polls it without blocking, and ends it with everything it started (a process group on
   Linux, a job object on Windows).
6. **The editor's Play**: an untitled project refuses with a notice to save it once (there is no
   folder to build in). While configuring or building the button reads Building and pressing it ends
   the build; while the game runs it reads Stop. A build that fails returns to Play with one line on
   stderr and no notice (0234). A close of the editor that goes ahead ends the game.

## Reasoning
The cook reads descriptions, and only `authoring` may (ADR-0151); C source is the one output the
game can compile in without parsing project text (0236), and hex floats make Play show the exact
scene the editor holds (0188). The world's type list must be one list or the cook names a type the
game never registered. `game` is separate from `app` because `app` owns no loop and knows no scene
types. A child process keeps the editor alive and responsive while the game runs, and ending the
process group is what stops Ninja as well as CMake. Rejected: cooking to a binary blob the game
reads (a second reader to keep in step), a game loop in `editor` (the game may not link it).

## Replaces
nothing
