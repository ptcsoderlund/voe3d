# cmake

The build functions every folder's CMakeLists.txt calls, and the build a game tree includes.

- `voe.cmake` — `voe_module()`, `voe_executable()`, the dependency map, the flag set, and the platform, render and editor bills.
- `game.cmake` — a game tree's build: descriptions and folder databases off, the engine's `game` folder, the executable `game` and its release install with the licences.
- `exports.cmake` — the `.def` the editor links on Windows, skipping the members that call a project's entry points.
