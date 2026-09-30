# 01 — The tank game does not build

## Seen
Opening `examples/tank_game` in the editor, building the project's code fails. `Build/build.log` shows the
same error in five of the game's own files — `tank_camera_system.c`, `tank_breakable.c`,
`tank_control_system.c`, `tank_shell_system.c`, `tank_lives_system.c`:

```
tank_camera_system.c:39:38: error: missing field 'needs' initializer [-Werror,-Wmissing-field-initializers]
   39 |                         &voe_ecs_runtime_only, NULL, NULL });
      |                                                           ^
1 error generated.
...
ninja: build stopped: subcommand failed.
```

The types that got a `needs` in this feature build; the ones that need nothing do not.

## Expected
The tank game builds and plays as before (step 7 of the feature's How to test).

More widely: a project's own code written before this feature builds without changes. A type that says
nothing about what it needs, needs nothing. Fixing only the tank game's files is not enough — anyone's
existing project would hit the same wall.

## How to reproduce
1. Build and start the editor.
2. Open `examples/tank_game`.
3. The project's code fails to build; read `examples/tank_game/Build/build.log`.
4. Also: take a project type written the pre-043 way (no mention of what it needs) in a fresh project; it
   must build too.
