# VOE3D

A general-purpose real-time 3D engine, written in C23 on Vulkan 1.3, for Windows and Linux
desktop. The repository holds the engine's folders, an editor for authoring scenes, and a dev
program that shows what the engine can currently do.

## What you need

Install these yourself. Everything the engine links against or ships is fetched by the build.

- Clang 19 or newer, as the GNU-driver `clang` (not `clang-cl`, MSVC or GCC)
- CMake 3.28 or newer
- Ninja
- `slangc`, the Slang shader compiler
- On Windows: the Windows SDK and MSVC C runtime (Visual Studio or its Build Tools)
- On Linux: the Wayland development package (`wayland-scanner` and the client library)

The Vulkan SDK is optional. It is an easy way to get `slangc`, and the engine uses its validation
layers when they are present. The Vulkan loader comes with your graphics driver.

## Build and check

    cmake --preset debug
    cmake --build --preset debug

A `release` preset exists as well. Output lands in `build/<preset>/`.

To verify the whole tree — tools, every folder building on its own, tests and the static
analyser — run from the repository root:

    cmake -P check.cmake

It prints one line per step and exits non-zero at the first failure. The tests of one folder:

    ctest --test-dir build/debug -R '^math/'

## Run

The editor opens a scene with a scene list, two scene views and an inspector:

    cmake --build --preset debug --target voe_editor
    ./build/debug/editor/voe_editor

With no arguments it opens the last project it had open, or an untitled cube
and light the first time it is ever run. `voe_editor <folder>` opens the
project at folder instead — a folder holding a `project.voe3d` file and the
scene it names. Which project was open last is remembered per person on the
machine, under `~/.config/voe3d/` on Linux, never inside a project.

The dev program opens a window on a world of models, text, sprites and interface panels:

    cmake --build --preset debug --target voe_dev
    ./build/debug/dev/voe_dev

In the dev program, Tab switches between orbiting the scene and flying the camera (W A S D, Q E,
Space, Ctrl, Shift and the mouse), P switches the present mode, and Escape hands the pointer back
or, pressed again, closes the window.

The editor can also draw one frame with no window at all and save it as a picture:

    ./build/debug/editor/voe_editor --capture <path> --size <W>x<H>

`--size` is optional and defaults to the window's own size; it is refused without `--capture`.

## Where to read more

- `CLAUDE.md` — the engine's rules, folder map and world conventions.
- `Agentic/decisions/` — why things are the way they are, one record per decision.
- Each folder's `<folder>.md` — what is in it.
