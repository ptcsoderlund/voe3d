# VOE3D
A general-purpose real-time 3D engine in C23 and Vulkan 1.3, with its editor and dev program built from the same
tree. Windows and Linux desktop; every card writes both, only Linux is checked, and Windows bugs are fixed as they arrive (0381). Clang 19+ as the GNU-driver `clang`,
CMake 3.28+, Ninja and `slangc` are installed by the programmer; everything else is fetched by the build.

The engine's standing rules — givens, rules 1–14 as code cites them, the scope of a card, the world's
conventions and the build shape — are decision 0168 in `Agentic/tech-lead/decisions/`; the planner names it on
every card. `ADR-NNNN` in code means the record of that number: 0001–0167 in `history/decisions/`, later ones in
`Agentic/tech-lead/decisions/`. "card NNN" means a file under `history/cards/`, "bug NNN" one under
`history/bugs/`, "spec NNN" the feature of that number under `Agentic/completed/` or `Agentic/kanban/`.

## Checks
- `[ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; top={folder}; top=${top%%/*}; [ "$top" != . ] && [ -f "$top/CMakeLists.txt" ] || exit 0; cmake --preset debug && cmake --build --preset debug --target voe_$top $(ninja -C build/debug -t targets all | grep -oE "^voe_test_${top}_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^$top/"`
- `[ -f ~/voe3d-scratch/env.sh ] && . ~/voe3d-scratch/env.sh; cmake -P check.cmake`

## Exempt
- `render/vulkan`
- `history`
- `build`
- `cmake-build-debug`
- `engine_assets`

## Never touch
- `history`
- `render/vulkan`
- `engine_assets`
