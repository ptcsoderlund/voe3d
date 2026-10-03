# 04 — Ship fails to build on Linux

## Seen
"I tried to ship the tank game but it failed on linux." The editor's Errors window showed the failure, and the
editor printed to its terminal:

```
voe_editor: building the release failed (exit 1), see /home/ptcsoderlund/Projekt/voe3d/examples/tank_game/Build/build.log
```

The build log, `examples/tank_game/Build/build.log`, stops in the Release build at:

```
FAILED: [code=1] scene/CMakeFiles/voe_scene.dir/src/transform_component.c.o
/home/ptcsoderlund/Projekt/voe3d/scene/src/transform_component.c:53:13: error: function 'is_finite' is not needed and will not be emitted [-Werror,-Wunneeded-internal-declaration]
   53 | static bool is_finite(voe_scene_transform t)
      |             ^~~~~~~~~
1 error generated.
ninja: build stopped: subcommand failed.
```

The command line shows `-O3 -DNDEBUG`. The tree's own checks build Debug and pass, so nothing caught this
before Ship ran.

## Expected
Ship builds the tank game in Release and installs it into `Build/ship/`, and the shipped program runs. A
change that breaks the Release build is caught when the change is made, not the next time someone ships.

## How to reproduce
1. Open `examples/tank_game` in the editor on Linux.
2. Press Ship.
3. The Errors window reports the build failure above, and nothing is installed into `Build/ship/`.
