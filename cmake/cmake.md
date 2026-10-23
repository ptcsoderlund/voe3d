# cmake

The build functions every folder's CMakeLists.txt calls, and the build a game tree includes.

- `voe.cmake` — `voe_module()`, `voe_executable()`, the dependency map, the flag set, and the platform, render and editor bills; render's shaders depend on every `.slangh` shader part.
- `game.cmake` — a game tree's build: the executable `game` with the cooked prefabs, landscapes and materials, its release install with the licences, and the project's sounds, models, pictures, levels files, cooked copies and `splashscreen.png`, copied beside the program.
- `exports.cmake` — the `.def` the editor links on Windows, skipping the members that call a project's entry points.
