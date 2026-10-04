# 04 — A game tree puts the chosen splash beside the program
folder: cmake
after: 03
decisions: 0168, 0346, 0356

## Change
- `cmake/game.cmake`, the executable branch beside the sounds, models and pictures: choose
  `<project>/Assets/splashscreen.png` when it exists, else `${VOE_ENGINE}/game/src/splashscreen.png`
  when that exists. Copy the chosen one to `${CMAKE_BINARY_DIR}/splashscreen.png` with a custom
  command in `game_files`, as the other files are copied, and install it to `.` under that name.
  When neither exists, remove a stale `${CMAKE_BINARY_DIR}/splashscreen.png` at configure.
  Add the engine's copy to the directory's CMAKE_CONFIGURE_DEPENDS when it exists, so its
  removal reconfigures; the project's is already watched by the `.png` glob. The project's file
  is still also copied and installed under `Assets/` as every `.png` is.
- The file's header comment: a paragraph on the splash: which file, where it goes, the
  fallback, and that the game reads it beside its program (0346, 0356).
- `cmake/cmake.md`: the `game.cmake` entry names the splash beside the program.

## Done when
In a `mktemp -d` project `P`: `P/Build/game/` holds empty `main.c`, `scene.c`, `prefabs.c`
and a `CMakeLists.txt` (cmake_minimum_required 3.28, `project(game C)`, `set(VOE_ENGINE
<this checkout>)`, `include(${VOE_ENGINE}/cmake/game.cmake)`). Configure it with
`-G Ninja -B P/Build/out` and build target `game_files`: `P/Build/out/splashscreen.png`
equals `game/src/splashscreen.png` (`cmp`). Then put any other PNG (e.g. `dev/src/logo.png`)
at `P/Assets/splashscreen.png`, build `game_files` again: the copy now equals that file.
