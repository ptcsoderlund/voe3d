# System

The map of VOE3D, a real-time 3D engine in C23 on Vulkan 1.3, its editor and dev program in one
tree. Every code folder is a standalone CMake project — `include/<folder>/`, `src/`, `tests/`, a
four-line `CMakeLists.txt` — built as `voe_<folder>`, linked as `voe::<folder>`, mapped by
`<folder>/<folder>.md`. Dependencies point down this list and never back up; `cmake/voe.cmake`
holds the allowed edges, so a change needing a new edge is a decision, not a card. The standing
rules are decision 0168.

- **base** — arenas, containers, strings, the two asserts, error codes, described structs.
- **math** — vectors and matrices spelled the way Slang spells them; knows no graphics API.
- **ecs** — entities, component tables, intent queues, a structural queue applied once a frame.
- **platform** — the one OS-aware folder: window, input, typed text, keymap, files, time.
- **scene** — transform, camera, light and identity as components with their systems.
- **assets** — glTF, images, fonts and the sectioned text format to CPU data; the JSON parser.
- **authoring** — scene and project text read and written; a game does not build it.
- **render** — the GPU layer and the only folder that names Vulkan: devices, resources, passes.
- **text** — a glyph atlas as a distance field and one mesh per text block; Oxanium, the one face.
- **ui** — immediate-mode GUI in millimetres: layout, panels, widgets, overlays, one draw.
- **theme** — a `.theme` file's bytes read into `ui`'s authored inputs; opens no file.
- **3d** — scene → draws → a render target: meshes, shapes, picking, selection outlines.
- **sprite** — a sprite is a plane in the world and hands back a material.
- **app** — parts a program calls in its frame loop: the frame, capture to a PNG, a headless start.
- **editor** — authoring a scene: top bar, Scene list, views, Inspector, files, Preferences. A leaf.
- **dev** — the program that shows what the engine can do; a leaf on all but editor and authoring.
- **testing** — the check macros a test links as `voe::testing`; not a library, not in the map.

## Decisions in force

0001–0167 stand in `history/decisions/`; 0168 on are indexed in `decisions/decisions.md`.

- 0168 — The Agentic workflow replaces specs; the engine's rules are one digest. On every card.
- 0001, 0008 — One repository of self-contained CMake folders, statically linked; no plugin.
- 0005, 0026, 0047 — Clang only, as the GNU-driver `clang` on both platforms, floor 19.
- 0021, 0023 — Tools installed by the programmer, dependencies fetched by the build.
- 0004, 0028, 0031, 0042 — No CI; `cmake -P check.cmake` verifies; tests are plain C under CTest.
- 0007, 0011, 0013, 0017 — The scene is an ECS; a component and its system are one module.
- 0014, 0029, 0032, 0034, 0041 — Naming carries the namespace; memory is arenas; failure is a value.
- 0022, 0121, 0135, 0151 — The folder map above; editor and dev are leaves; a game omits authoring.
- 0033, 0035 — Right-handed +Y up −Z forward, reversed depth, one Y flip; `math` speaks Slang.
- 0015, 0040, 0046 — Vulkan without an SDK; shaders in Slang, compiled at build time and embedded.
- 0113, 0130 — A change owns the call sites it breaks downstream; Linux alone verifies a card.
- 0120, 0133, 0173 — Every code folder carries a `.md`; `src/` and `tests/` list their own files.
- 0169 — A keysym is a number as often as a name, a key has four levels, and AltGr is a place.
- 0170, 0171, 0172, 0178 — A theme is read in `theme`, derived in `ui` in OKLab; the nearest wins.
- 0174, 0175, 0176 — 006's cards merged back; a new folder registers itself; `theme` sees `render`.
- 0177 — To see what was drawn, render to a PNG; named on cards that change what is drawn.
- 0180, 0181 — A Wayland window draws at the compositor's fractional scale, in buffer pixels.
- 0182, 0183, 0184 — Editor text keeps scaling and hard edges; the cutoff sits half a pixel out.
- 0185 — Oxanium is the only font; a theme naming another gets it, and there is no font choice.
- 0186, 0187, 0188 — Toward a coin game in the project's own C; Play cooks the scene as it is.
- 0189, 0191 — An entity is only a number; a shape has a described linear colour until materials.
- 0190 — The world owns which rows exist, through `ecs`'s structural queue; a system their values.
- 0192 — `ui` holds one keyboard focus; the colour picker is a `ui` panel the caller places.
- 0193, 0195, 0198 — The editor changes structure through the queue; a named field is a dropdown.
- 0194, 0196 — One hue authored as `hue=`, roles differing only in lightness, state drawn inverted.
- 0197 — Slider values are remembered per theme in one settings file; a theme file is never written.
- 0199, 0200 — An overlay belongs to its widget, swallows the cursor, takes the side that fits.
- 0201 — The engine takes the fastest card — never software silently — and says which it took.
- 0202, 0203 — A click picks by a ray cast against the shapes' triangles; the outline is quads.
