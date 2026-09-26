# Onboarding

How to build VOE3D, how the tree is put together, and how a game gets from the editor to a program
of its own. The [README](README.md) covers what this project is (and isn't); this file is the
technical part.

## What you need

Install these yourself. Everything else the engine needs is in the tree.

- Clang 19 or newer, as the GNU-driver `clang` (not `clang-cl`, MSVC or GCC)
- CMake 3.28 or newer
- Ninja
- `slangc`, the Slang shader compiler
- On Windows: the Windows SDK and MSVC C runtime (Visual Studio or its Build Tools)
- On Linux: the Wayland development package (`wayland-scanner` and the client library)

The Vulkan SDK is optional. It is an easy way to get `slangc`, and the engine uses its validation
layers when they are present. The Vulkan loader comes with your graphics driver.

## Building with CMake

There are two presets, `debug` and `release`. Each builds into `build/<preset>/`:

    cmake --preset debug
    cmake --build --preset debug

    cmake --preset release
    cmake --build --preset release

Every folder is its own target, named `voe_<folder>`, so you can build just the part you care about:

    cmake --build --preset debug --target voe_math

Tests are plain C programs, one per module, registered with CTest under their folder's name:

    ctest --test-dir build/debug                 # everything
    ctest --test-dir build/debug -R '^math/'     # one folder

To verify the whole tree — the tools, every folder building on its own, the tests and the static
analyser — run from the repository root:

    cmake -P check.cmake

It prints one line per step and stops with a non-zero exit at the first failure.

**Editor setup.** Each folder gets its own `compile_commands.json` copied next to its
`CMakeLists.txt` after a build, so clangd works when you open a single folder. For the whole tree,
point your editor at `build/debug/`.

## How the engine is put together

VOE3D is a set of folders, and each folder is a static library:

    math/
      math.md             what is in the folder
      CMakeLists.txt      four lines: a call to voe_module()
      include/math/       the public API: the only headers other folders may include
      src/                the implementation, private to the folder
      tests/              one test program per module

A folder links as `voe::<folder>` and is included as `#include <math/float3.h>`. Anything in `src/`
is private to its folder. **The headers under `include/` are the engine's public API, and nothing
else is.**

Dependencies only point one way. `cmake/voe.cmake` holds the allowed edges, and a `DEPENDS` outside
them fails at configure time. From the bottom up:

| Folder     | What it is                                                                 |
|------------|----------------------------------------------------------------------------|
| `base`     | arenas, containers, strings, asserts, error codes, described structs       |
| `math`     | vectors and matrices, spelled the way Slang spells them                    |
| `ecs`      | entities, component tables, intent queues                                  |
| `platform` | the only OS-aware folder: window, input, files, clock, processes           |
| `scene`    | transform, lens, light, identity                                           |
| `physics`  | colliders, overlap queries, the kinematic body                             |
| `assets`   | glTF, images, fonts and text formats read into CPU data                    |
| `authoring`| scene and project text read and written, a world cooked to C               |
| `render`   | the GPU layer, and the only folder that names Vulkan                       |
| `text`     | glyph atlas and text meshes                                                |
| `ui`       | immediate-mode GUI laid out in millimetres                                 |
| `theme`    | `.theme` files read into `ui`'s inputs                                     |
| `3d`       | scene to draws to a render target: meshes, picking, gizmo, shadows         |
| `sprite`   | sprites as planes in the world                                             |
| `app`      | the frame loop, PNG capture, headless start                                |
| `game`     | a shipped game's loop: fixed 1/60 s steps, a frame, the project's code     |

A few conventions you'll see everywhere:

- **Names carry the namespace:** `voe_<folder>_<thing>`, because C has no namespaces.
- **Memory comes from arenas,** allocated up front and released all at once, not object by
  object.
- **Failure is a value you get back,** and asserts are only for programmer errors.
- **World axes:** +Y is up and −Z is forward. Positions are `double`, and the GPU gets them
  relative to the camera. Depth is reversed.
- **Shaders are Slang,** compiled at build time and embedded into the binary with `#embed`.

## The editor and the dev program use the public API too

`editor/` and `dev/` are programs, not libraries. They sit at the top of the tree, and nothing
depends on them. They use the engine only through the same `include/` headers that any game would
use. No engine folder contains code meant only for the editor. If the editor needs something the
engine doesn't have yet, that feature is added to the engine folder where it belongs.

- **`dev`** shows what the engine can do: models, text, sprites and panels in one world.

      cmake --build --preset debug --target voe_dev
      ./build/debug/dev/voe_dev

  Tab switches between orbiting and flying (W A S D, Q E, Space, Ctrl, Shift and the mouse), P
  switches the present mode, and Escape releases the pointer or, pressed again, quits.

- **`editor`** is where you author scenes: a scene list, two scene views, a gizmo and an inspector.

      cmake --build --preset debug --target voe_editor
      ./build/debug/editor/voe_editor                  # last project, or a fresh one
      ./build/debug/editor/voe_editor examples/coin_game

  It can also render one frame headless and save it as a PNG:

      ./build/debug/editor/voe_editor --capture <path> --size <W>x<H>

  `--size` defaults to the window's own size, and is only allowed together with `--capture`.

## From the editor to a program you can run

The editor doesn't only edit scenes. It builds your project into a **standalone executable**, with
no editor inside it.

**A project** is a folder:

    my_game/
      project.voe3d       names the scene to start in
      main.scene          the scene, as text
      Code/               your components and systems, in C
      Build/              everything generated (git-ignored)

`Code/` is written against `game/project.h`. A project defines four entry points:

- **register** — declares the project's component types, which then appear in the Inspector.
- **systems before the move** — run every fixed step, before physics moves the bodies.
- **systems after the move** — run in the same step, after the move (a follow camera goes here).
- **interface** — runs once a frame and draws menus and the HUD with `ui`.

`examples/coin_game/` and `examples/capsule/` show all four in use.

**Refresh** builds `Code/` as a shared library (`libproject.so`, or `project.dll` on Windows) into
`Build/editor/`. The editor loads it, so your components appear in the Inspector and can be placed
in the scene. Build errors show up in an Errors panel, and the full log is in `Build/build.log`.

**Play** does the following:

1. Writes a small game tree into `Build/game/`: a `CMakeLists.txt`, a `main.c` that calls
   `voe_game_run()`, and a `scene.c` that holds the current scene (unsaved edits included),
   converted to C.
2. Builds the engine from source together with your `Code/` into `Build/debug/game`
   (`game.exe` on Windows). The game links the `game` folder and what it depends on. It does not
   link the editor or `authoring`.
3. Starts that program as a separate process. Press Play again to stop it.

The result is a program that runs on its own. Some limits for now:

- **Debug build.** The game builds with debug info, so you can attach a debugger or open `Build/`
  in an IDE.
- **Needs the engine source and your tools.** The editor finds both through paths recorded when
  the editor itself was built, so the engine's source tree has to stay where it was.
- **No asset cooking yet.** Packaging assets for shipping is planned but not built.

## Where to read more

- Each folder's `<folder>.md` says what is in it, and `src/src.md` and `tests/tests.md` list every
  file.
- `Agentic/tech-lead/system.md` is the folder map, with the decisions every change relies on.
- `Agentic/tech-lead/decisions/0168-…` holds the engine's standing rules and conventions in one
  record.
- Decisions ("ADR-NNNN" in the code) explain why things are the way they are. 0001–0167 are in
  `history/decisions/`, and later ones are in `Agentic/tech-lead/decisions/`.
- `Agentic/` holds the state of the planner/coder workflow: work orders, the feature being built,
  and the features already accepted. `history/` is the archive of earlier cards and bugs.
