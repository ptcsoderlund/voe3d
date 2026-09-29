# 19 — The game build compiles the cooked prefabs
folder: cmake
decisions: 0168, 0283, 0237

## Change
Needs card 18. 0283 point 9.

- `cmake/game.cmake`: in the default mode (not `VOE_GAME_LIBRARY`), the executable `game` is
  built from `prefabs.c` beside `main.c` and `scene.c` in the tree's source folder. The library
  mode is unchanged: a project library calls `voe_game_project_spawn`, which the editor defines,
  and never the cooked table. The header's opening paragraph names `prefabs.c` as defining
  `voe_game_prefabs_cooked` (game/prefabs.h), and the mode paragraph says what each mode
  compiles.
- `cmake/cmake.md`: the entry for `game.cmake`, if it lists the tree's files.

## Done when
From the repository root, this exits 0 (a hand-made tree with no code and no prefabs links):

```sh
d=$(mktemp -d) &&
printf 'cmake_minimum_required(VERSION 3.28)\nproject(t C)\nset(VOE_ENGINE "%s")\n' "$PWD" > "$d/CMakeLists.txt" &&
printf 'include("${VOE_ENGINE}/cmake/game.cmake")\n' >> "$d/CMakeLists.txt" &&
printf '#include <game/run.h>\nint main(void) { return voe_game_run("t"); }\n' > "$d/main.c" &&
printf '#include <game/scene.h>\nbool voe_game_scene_build(voe_ecs_world *w) { (void)w; return true; }\n' > "$d/scene.c" &&
printf '#include <game/prefabs.h>\nconst voe_game_prefabs voe_game_prefabs_cooked = { 0 };\n' > "$d/prefabs.c" &&
cmake -S "$d" -B "$d/b" -G Ninja -DCMAKE_C_COMPILER=clang &&
cmake --build "$d/b" --target game
```
