# Voluntary Overtime Engine 3D

![Voluntary Overtime Engine 3D](engine_assets/Engine%20images/Primary.png)

A general-purpose real-time 3D engine, written in C23 on Vulkan 1.3, for Windows and Linux
desktop. The repository holds the engine, an editor for authoring scenes that builds your project
into a standalone game, and a dev program that shows what the engine can currently do.

## Read this first

**This is a hobby project, built for fun.** It is not a product, and nothing here is a promise:

- **No stability.** APIs, file formats and folder layout change whenever I feel like it.
  Versions are `0.x` and will likely stay that way.
- **Linux is the tested platform.** Windows is meant to work, but it is not tested on every change
  and may be broken at any given time. There are no Windows downloads: to make a Windows game,
  build the engine and editor from source on Windows.
- **No pull requests.** They will be closed unopened. Fork it and do whatever you like instead —
  that is what the license is for.
- **Issues may go unanswered.** Feel free to open one; just don't expect a reply or a fix.
- **I don't write the code. AI agents do.** Claude agents plan, write and test every change
  through a planner/coder workflow. My part is deciding what gets built and trying the result in
  the editor. I don't read most of the code, and technically I'm out of my depth in much of it.
  So don't take anything here as how an expert would do it. It may be good, it may be bad, and
  I often couldn't tell you which. The workflow's records live in `Agentic/` and `history/`.

If you find something useful in here, take it. If you want an engine to ship a game on, use
one with a team behind it.

## Quick start

With Clang 19+, CMake 3.28+, Ninja and `slangc` installed:

    cmake --preset debug
    cmake --build --preset debug
    ./build/debug/editor/voe_editor examples/coin_game

Press Play in the editor to build and run the coin game as a program of its own.

**[ONBOARDING.md](ONBOARDING.md)** has the rest: the full list of what to install, the CMake
presets and targets, tests, how the engine's folders and public API fit together, and how the
editor turns a project into an executable.

## Contact

Per Söderlund — [ptcsoderlund@gmail.com](mailto:ptcsoderlund@gmail.com). Email is a better way to
reach me than an issue.

## License

voe3d is released under the MIT License; see [`LICENSE`](LICENSE).

These files are not covered by it and keep their own licenses:

- `render/vulkan/` — the Vulkan headers, © The Khronos Group Inc., Apache-2.0.
- `text/fonts/Oxanium-Regular.ttf` — the Oxanium font, SIL Open Font License 1.1; see
  `text/fonts/OFL.txt`.
