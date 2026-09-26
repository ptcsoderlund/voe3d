# 0264 — Ship builds the game tree in release and installs it into `Build/ship/<name>/`
date: 2026-09-26
by: planner

## Decision
For 031, filling in what the feature and 0235 ("milestone 6 builds the same tree in release")
leave to the planner:

1. **One tree, a third binary folder.** Ship writes the same `<project>/Build/game/` Play writes
   (the scene cooked as it is at the press, unsaved edits included, 0188) and configures it into
   `<project>/Build/release/` with `CMAKE_BUILD_TYPE=Release`: optimised, `NDEBUG`, no `-g`, no
   descriptions and no `authoring` (as every game tree). Outside Windows the program is linked
   with `-s`, so it carries no symbols either.
2. **The shipped folder is `<project>/Build/ship/<name>/`**, `<name>` the project's folder name,
   holding the program `<name>` (`<name>.exe` on Windows) and the two licences the program
   carries: the engine's `LICENSE` as `voe3d-LICENSE.txt` and Oxanium's `OFL.txt` as
   `Oxanium-OFL.txt`. Shaders and the font are embedded (ADR-0046, 0185), so nothing else is read
   at run time; the program needs the system's C library, `libwayland-client` and a Vulkan
   loader and driver, which a Linux desktop has. It is inside `Build/`, which every project's
   `.gitignore` already lists, so no ignore file changes.
3. **`cmake/game.cmake` owns what is shipped**: install rules for the program (renamed to
   `VOE_GAME_NAME`, default `game`, which the tree's `CMakeLists.txt` sets) and the two licences.
   The editor runs four child processes in turn: configure (once), build `game`,
   `cmake -E rm -rf` the shipped folder, `cmake --install` into it. The old folder is removed
   only after a build that succeeded, so a failed Ship leaves the last one as it was.
4. **Ship is Play's shape in the editor**: `editor/src/ship.h`, a top-bar button between Refresh
   and Preferences reading Ship or Shipping; a press while shipping does nothing. With code in
   `Code/` it refreshes first, as Play does, so the cook sees the types the game compiles. Success
   sets the notice to the shipped folder's path; a failed step writes `Build/build.log` and shows
   the Errors panel. An untitled project refuses. A close that goes ahead, New and Open end it.
5. **One build at a time in `Build/game/`.** Ship refuses with a notice while Play configures or
   builds or a refresh runs for Play; during any other refresh it waits for it. Play and Refresh
   refuse with a notice while Ship runs. A running game is never stopped by Ship.

## Reasoning
The same tree and the same cook are what make the shipped game play as Play does (0235's one
cook). `cmake --install` and `cmake -E rm -rf` are CMake's own, so no platform code is added for
copying or removing folders. Rejected: a new top-level `Ship/` folder (existing `.gitignore`s are
never touched, 0237 point 3); copying the program from the editor (a second place that knows the
program's name); building concurrently with Play (both would write `Build/game/` and one log).

## Replaces
nothing
