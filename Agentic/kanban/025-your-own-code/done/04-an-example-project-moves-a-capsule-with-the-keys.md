# 04 — An example project moves a capsule with the keys and a camera follows it
folder: game
decisions: 0168, 0239, 0242

## Change
Point 10 of 0242: `game/example/` (new), data plus `Code/` (new code folder). Read
`game/include/game/project.h`, `base/describe.h`, `scene/transform_system.h`,
`platform/input.h`, and for the data `authoring/scene_write.h` and the header of
`editor/src/project.h` (what project.voe3d holds).

- `game/example/project.voe3d`, `game/example/main.scene` — written by hand in the format those
  headers give: a floor (a cube shape scaled flat), a capsule (`VOE_3D_SHAPE_CAPSULE`) standing on
  it, the camera above and behind it looking at it, one light. Engine components only (How to test
  step 1).
- `game/example/.gitignore` — `/Build/` and `/Cache/` if the editor makes one.
- `game/example/Code/` — one header per component (struct, key, `VOE_BASE_DESCRIBE_STRUCT`, a
  register call) and one `.c` per system; each file's header comment says what it teaches:
  - `keyboard_input` {move, FLOAT2, read-only}; `keyboard_system` writes its own rows from
    W/A/S/D via `voe_platform_input_key_down` (x right, y forward, each −1..1); a NULL window
    reads nothing;
  - `player` {speed, float, default 2}; `player_system`, for every entity with player and
    keyboard_input, submits a transform moved on XZ by move × speed × seconds (W is −Z, D is +X);
  - `follow_camera` {target ENTITY, distance float, default 6}; its system puts the entity's
    position at the target's minus forward × distance, forward being its own rotation applied to
    (0,0,−1) (a static helper if math/quat.h has none), rotation kept; a dead target is skipped;
  - `project.c` — the two entry points: register the three through
    `voe_game_project_component` with `VOE_GAME_PROJECT_DESCRIPTION` and menus "Keyboard Input",
    "Player", "Follow Camera"; run keyboard, player, follow in that order.
  - `Code/Code.md` — "# Code", an entry per file.
- `game/example/example.md` — "# example": what the project is, the data files in a sentence, and
  the entry for `Code`.
- `game/game.md` — the entry for `example`.

## Done when
1. `cmake --preset debug` exits 0; tool values from `build/debug/generated/editor/toolchain.h`.
2. In `t=$(mktemp -d)`: `$t/src/` with CMakeLists.txt, main.c and scene.c as in card 03 step 2,
   `VOE_PROJECT_CODE` set to this repository's `game/example/Code`, and scene.c including
   `keyboard_input.h`, `player.h`, `follow_camera.h`. Configured with the tool values into `$t/g`,
   `cmake --build $t/g --target game` exits 0; into `$t/l` with `-DVOE_GAME_LIBRARY=ON`,
   `cmake --build $t/l --target project` exits 0 and `nm -D --defined-only $t/l/libproject.so`
   lists both entry points.
3. `git status --porcelain` lists no `compile_commands.json` and nothing under `game/example/Build`.
4. `checks.sh --folder game` prints `FINDINGS: 0`.
