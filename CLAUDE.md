# VOE3D
A general-purpose real-time 3D engine in C23 and Vulkan 1.3, with its editor and dev program built from the same
tree. Windows and Linux desktop; Linux alone verifies a card (ADR-0130). Clang 19+ as the GNU-driver `clang`,
CMake 3.28+, Ninja and `slangc` are installed by the programmer; everything else is fetched by the build.

The engine's standing rules — givens, rules 1–14 as code cites them, the scope of a card, the world's
conventions and the build shape — are decision 0168 in `Agentic/tech-lead/decisions/`; the planner names it on
every card. `ADR-NNNN` in code means the record of that number: 0001–0167 in `history/decisions/`, later ones in
`Agentic/tech-lead/decisions/`. "card NNN" means a file under `history/cards/`, "bug NNN" one under
`history/bugs/`, "spec NNN" the feature of that number under `Agentic/completed/` or `Agentic/kanban/`.

## Checks
- `cmake --preset debug && cmake --build --preset debug --target voe_{folder} && ctest --test-dir build/debug -R '^{folder}/'`
- `cmake -P check.cmake`

## Seeing what was drawn
To check layout, render to a PNG rather than taking a screenshot of the programmer's display:
`./build/debug/editor/voe_editor --capture <path>.png [--size <W>x<H>]` draws one frame with no window,
writes it and exits, then read the PNG. A program of your own does the same through
`voe_app_new_headless` and `voe_app_capture_png` (ADR-0157).

## Exempt
- `render/vulkan`
- `history`
- `build`
- `cmake-build-debug`

## Never touch
- `history`
- `render/vulkan`
