# 01 — The tank game does not build on Windows

## Seen
Opening `examples/tank_game` in the editor on Windows, the editor says building the project's code failed.
`examples/tank_game/Build/build.log` shows every file in `Code/` failing the same two ways (clang, `-Werror`,
`-DVOE_BASE_IMPORTING`):

```
In file included from .../examples/tank_game/Code/tank_turret_system.c:30:
In file included from .../game/include\game/project.h:62:
In file included from .../game/include\game/prefabs.h:23:
In file included from .../game/include\game/scene.h:26:
.../3d/include\3d/shape_component.h:81:1: error: initializer element is not a compile-time constant
   81 | VOE_BASE_DESCRIBE_STRUCT_NAMED(voe_3d_shape, VOE_3D_SHAPE_FIELDS,
.../base/include\base/describe.h:341:13: note: expanded from macro 'VOE_BASE_DESCRIBE_NAMES_ROW_'
  341 |                 .values = (values_),                                          \
.../physics/include\physics/collider_component.h:57:1: error: initializer element is not a compile-time constant
2 errors generated.
ninja: build stopped: subcommand failed.
```

The include chain is new with this feature: `game/project.h` now reaches `game/prefabs.h`. Every agent check passed
because they run on Linux, where the same code builds.

## Expected
The tank game's code builds on Windows as it did before this feature, and Play works there.

## How to reproduce
1. On Windows, build the editor from this branch.
2. Open `examples/tank_game` in it.
3. The editor reports that building the project's code failed; read `examples/tank_game/Build/build.log`.
