# System

The map of VOE3D, a real-time 3D engine in C23 on Vulkan 1.3 with its editor and dev program in the same tree.
Every code folder is a standalone CMake project — `include/<folder>/`, `src/`, `tests/`, a four-line
`CMakeLists.txt` — built as `voe_<folder>` and linked as `voe::<folder>`. Dependencies point down this list and
never back up; `cmake/voe.cmake` holds the allowed edges and refuses any other, so a change that needs a new edge
is a decision, not a card. The engine's standing rules, numbered as code cites them, are decision 0168.

- **base** — memory as arenas, containers, strings, the two asserts, the error codes, the report call and the description of a struct's fields. Depends on nothing. Open `base/base.md`.
- **math** — vectors and matrices spelled the way Slang spells them, pure data with support functions; knows no graphics API. Depends on nothing. Open `math/math.md`.
- **ecs** — entities, components as tables, intent queues, and the structural queue that adds and removes rows and destroys entities once a frame; every described type registers a default row. On base. Open `ecs/ecs.md`.
- **platform** — window, input, typed text and the Wayland keymap, files, folders, paths, time. The only OS-aware folder; Wayland and Win32 backends as equals, and a Wayland window draws at the compositor's fractional scale with its size and pointer in buffer pixels. On base. Open `platform/platform.md`.
- **scene** — transform, camera, light and identity as components with their systems. On ecs, math, base. Open `scene/scene.md`.
- **assets** — glTF, images, fonts and the sectioned text format to CPU data; the JSON parser. Never recurses over a file. On platform, math, base. Open `assets/assets.md`.
- **authoring** — scene and project text read and written, world ↔ scene file; a game does not build it. On scene, ecs, assets, math, base. Open `authoring/authoring.md`.
- **render** — the GPU layer and the only folder that names Vulkan: device, memory, resources by id, pipelines, targets, passes, present. Headers vendored in `render/vulkan/`, loader opened by name, every call through one resolved function table. On platform, math, base. Open `render/render.md`.
- **text** — a glyph atlas as a three-channel distance field and one mesh per text block; Oxanium embedded, the one face, named by an enum (ADR-0185). On render, math, base. Open `text/text.md`.
- **ui** — immediate-mode GUI in millimetres: flexbox-like layout, panels, buttons, number boxes dragged or typed into, text fields sharing one keyboard focus, scroll areas, a swatch, a colour picker and a slider, one draw from an element buffer; a theme's palette derived in OKLab from five authored values, every role in the one hue and state drawn inverted, the nearest theme in force winning. On render, text, math, base. Open `ui/ui.md`.
- **theme** — one `.theme` file's bytes read into `ui`'s authored inputs, its colour authored as `hue=`, a typeface and a display name, or refused with the line named; opens no file and derives nothing. On ui, text, assets, math, base, and render for its test's device only. Open `theme/theme.md`.
- **3d** — the 3D renderer: scene → draws → a render target, meshes, materials, built-in shapes (cube, capsule, cylinder) each in a described linear colour. On render, scene, ecs, assets, math, base. Open `3d/3d.md`.
- **sprite** — a sprite is a plane in the world and hands back a material. On 3d, render, math, base. Open `sprite/sprite.md`.
- **app** — parts a program calls in its own frame loop: the frame, capture to a PNG, a windowless start. On 3d, render, assets, platform, scene, ecs, math, base. Open `app/app.md`.
- **editor** — the program a person opens to author a scene: top bar, Scene list with Add, scene views, an Inspector that types values, picks colours, adds and removes components and duplicates and deletes entities, file browser, projects opened and saved, themes chosen in Preferences and re-read live with two sliders for their contrast and surface separation, `--capture`. A leaf; nothing names it. Open `editor/editor.md`.
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
- 0172, 0178 — A theme is one file in a themes folder, remembered by its file name; Near black and Near white are built in, the light one remembered as `near_white`.
- 0173 — Every folder holding code carries `<folder>.md` as an index; `src/` and `tests/` list their own files.
- 0175, 0176 — A card that creates a folder registers it in `cmake/voe.cmake` and the root `CMakeLists.txt`; `theme`'s row names `render`, for its test's headless device.
- 0177 — To see what was drawn, render to a PNG; the planner names this on cards that change what is drawn.
- 0180 — A Wayland window draws at the compositor's fractional scale, and its pixels are the buffer's.
- 0182, 0184 — Editor text keeps scaling with the window and keeps hard edges; the glyph cutoff sits half a screen pixel out, less one byte step, so no stroke vanishes.
- 0185 — Oxanium is the only font; a theme naming any other gets it, and the editor offers no font choice.
- 0186, 0187, 0188 — Work moves toward a coin-collecting capsule game; its logic is the project's own C, and Play cooks the scene on screen and starts it as a separate program.
- 0189, 0191 — An entity is only a number and starts at the origin; a material is an instance of a shader, and until then a shape has only a described linear colour drawn through its object record; capsule and cylinder are built-in kinds.
- 0190 — The world owns which rows exist, through `ecs`'s structural queue applied once a frame; a system owns their values. Amends rule 3.
- 0192 — `ui` holds one keyboard focus for fields and typed number boxes; the colour picker is a `ui` panel the caller places.
- 0193, 0195 — The editor changes structure through the queue; new ids are max+1 and names like "Cube 2"; the Inspector shows described components only, and a named-list field such as a shape's kind as a dropdown.
- 0194, 0196 — Colour carries no meaning in an editor's interface: a theme has one hue, authored as `hue=`, roles differ only in lightness, and what is held, dragged or selected is drawn in the palette's `inverse` with its text in `inverse_ink`; a slider is a number box with a thumb. Replaces the accent of 0171 and 0172.
- 0197 — The editor remembers a person's contrast and surface separation per theme in `<settings>/voe3d/theme_scalars`, written when a drag ends and on Reset; a theme file is never written.
