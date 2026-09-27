# 02 — A scene with models does not build into a game

## Seen
With `tank_body.glb` and `tank_head.glb` placed in `examples/tank_game`, building the game fails.
`examples/tank_game/Build/build.log`:

```
[0/2] Re-checking globbed directories...
-- GLOB mismatch!
The following files were added:
  +.../examples/tank_game//Build/debug/Assets/tank_body.glb
  +.../examples/tank_game//Build/debug/Assets/tank_head.glb
[1/2] Re-running CMake...
...
FAILED: [code=1] CMakeFiles/game.dir/scene.c.o
.../examples/tank_game/Build/game/scene.c:30:67: error: use of undeclared identifier 'voe_3d_model_key'; did you mean 'voe_3d_panel_key'?
   30 |         if (!voe_ecs_component_add(world, voe_ecs_component_type(world, &voe_3d_model_key), e[3], &(voe_3d_model){ .path = "Assets/tank_body.glb" }))
.../examples/tank_game/Build/game/scene.c:30:94: error: use of undeclared identifier 'voe_3d_model'; did you mean 'voe_3d_panel'?
(the same two errors again at line 36 for Assets/tank_head.glb)
4 errors generated.
ninja: build stopped: subcommand failed.
```

The GLOB mismatch lines are CMake noticing the newly copied `.glb` files and re-running; that part
went through. The failure is the generated `scene.c`, which names the model component without
what declares it.

## Expected
A scene with placed models builds, and the game plays and ships with the models drawn, lit and
shadowed as in the editor (036 How to test, step 8). No build noise the user has to wonder about.

## How to reproduce
1. Open `examples/tank_game` in the editor.
2. Place `tank_body.glb` and `tank_head.glb` from the Assets panel into a view. Save.
3. Press Play (or Ship).
4. The build fails; `examples/tank_game/Build/build.log` shows the errors above.
