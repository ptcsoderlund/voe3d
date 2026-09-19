# System

The map of VOE3D, a real-time 3D engine in C23 on Vulkan 1.3 with its editor and dev program in the same tree.
Every code folder is a standalone CMake project — `include/<folder>/`, `src/`, `tests/`, a four-line
`CMakeLists.txt` — built as `voe_<folder>` and linked as `voe::<folder>`. Dependencies point down this list and
never back up; `cmake/voe.cmake` holds the allowed edges and refuses any other, so a change that needs a new edge
is a decision, not a card. The engine's standing rules, numbered as code cites them, are decision 0168.

- **base** — memory as arenas, containers, strings, the two asserts, the error codes, the report call and the description of a struct's fields. Depends on nothing. Open `base/base.md`.
- **math** — vectors and matrices spelled the way Slang spells them, pure data with support functions; knows no graphics API. Depends on nothing. Open `math/math.md`.
- **ecs** — entities, components as tables, intent queues. On base. Open `ecs/ecs.md`.
- **platform** — window, input, typed text and the Wayland keymap, files, folders, paths, time. The only OS-aware folder; Wayland and Win32 backends as equals. On base. Open `platform/platform.md`.
- **scene** — transform, camera, light and identity as components with their systems. On ecs, math, base. Open `scene/scene.md`.
- **assets** — glTF, images, fonts and the sectioned text format to CPU data; the JSON parser. Never recurses over a file. On platform, math, base. Open `assets/assets.md`.
- **authoring** — scene and project text read and written, world ↔ scene file; a game does not build it. On scene, ecs, assets, math, base. Open `authoring/authoring.md`.
- **render** — the GPU layer and the only folder that names Vulkan: device, memory, resources by id, pipelines, targets, passes, present. Headers vendored in `render/vulkan/`, loader opened by name, every call through one resolved function table. On platform, math, base. Open `render/render.md`.
- **text** — a glyph atlas as a three-channel distance field and one mesh per text block; Oxanium embedded, the one face, named by an enum (ADR-0185). On render, math, base. Open `text/text.md`.
- **ui** — immediate-mode GUI in millimetres: flexbox-like layout, panels, buttons, number boxes, text fields, scroll areas, one draw from an element buffer; a theme's palette derived in OKLab from five authored values, the nearest theme in force winning. On render, text, math, base. Open `ui/ui.md`.
- **theme** — one `.theme` file's bytes read into `ui`'s authored inputs, a typeface and a display name, or refused with the line named; opens no file and derives nothing. On ui, text, assets, math, base, and render for its test's device only. Open `theme/theme.md`.
- **3d** — the 3D renderer: scene → draws → a render target, meshes, materials, built-in shapes. On render, scene, ecs, assets, math, base. Open `3d/3d.md`.
- **sprite** — a sprite is a plane in the world and hands back a material. On 3d, render, math, base. Open `sprite/sprite.md`.
- **app** — parts a program calls in its own frame loop: the frame, capture to a PNG, a windowless start. On 3d, render, assets, platform, scene, ecs, math, base. Open `app/app.md`.
- **editor** — the program a person opens to author a scene: top bar, Scene list, scene views, Inspector, file browser, projects opened and saved, themes chosen in Preferences and re-read live, `--capture`. A leaf; nothing names it. Open `editor/editor.md`.
- **dev** — the program that shows what the engine can do; a leaf on every folder but editor and authoring. Open `dev/dev.md`.
- **testing** — the check macros a test links as `voe::testing`; not a library and not in the dependency map. Open `testing/testing.md`.

## Decisions in force

Every record 0001–0167 in `history/decisions/` stands unless one says it is superseded; these are the ones a card leans on most, and 0168 digests them.

- 0168 — The Agentic workflow replaces specs, and the engine's rules are one digest. Name it on every card.
- 0001, 0008 — One repository of self-contained CMake folders, statically linked; no runtime plugin boundary.
- 0005, 0026, 0047 — Clang only, as the GNU-driver `clang` on both platforms, floor 19.
- 0021, 0023 — Tools are installed by the programmer, dependencies fetched by the build; write it ourselves.
- 0004, 0028, 0042, 0031 — No CI; `cmake -P check.cmake` is the verification, analyser as step 7, tests as plain C under CTest.
- 0007, 0011, 0013, 0017 — The scene is an ECS; a component and its system are one module; intent is a datatype; no system calls a system.
- 0022, 0121, 0135, 0151 — The folder map above; the editor and dev are leaves; `app` is parts a program calls; `authoring` is not built by a game.
- 0014, 0029, 0032, 0034, 0041 — Naming carries the namespace and the `voe_` prefix; memory is arenas; implement on demand; recoverable failure is a returned value.
- 0033, 0035 — Right-handed +Y up −Z forward, column vectors, row-major storage, reversed depth, one Y flip in the viewport; `math` speaks Slang.
- 0040, 0046, 0015 — Vulkan without an SDK; shaders in Slang, compiled at build time and embedded.
- 0113, 0130 — A change owns the call sites it breaks downstream; Linux alone verifies a task.
- 0120, 0133 — A folder `.md` is a present-tense map that delegates its subfolders.
- 0169 — A keysym is a number as often as a name, a key has four levels, and AltGr is a place the keymap names.
- 0170 — A theme is read in `theme`, derived in `ui`, and the nearest one wins.
- 0171 — The palette is derived in OKLab from one colour and two numbers, and dark mode clamps chroma.
- 0172 — A theme is one file in a themes folder, and a program remembers which by its file name.
- 0173 — Every folder holding code carries `<folder>.md` as an index; `src/` and `tests/` list their own files.
- 0175 — A card that creates a folder registers it in `cmake/voe.cmake` and the root `CMakeLists.txt`.
- 0176 — `theme`'s row names `render`, for its test's headless device.
- 0177 — To see what was drawn, render to a PNG; the planner names this on cards that change what is drawn.
- 0178 — Two built-in themes, Near black and Near white, the light one remembered as `near_white`.
- 0185 — Oxanium is the only font; a theme naming any other gets it, and the editor offers no font choice.
