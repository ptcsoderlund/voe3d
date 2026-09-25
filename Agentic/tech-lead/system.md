# System

The map of VOE3D, a real-time 3D engine in C23 on Vulkan 1.3, its editor and dev program in one
tree. Every code folder is a standalone CMake project — `include/<folder>/`, `src/`, `tests/`, a
four-line `CMakeLists.txt` — built as `voe_<folder>`, linked as `voe::<folder>`, mapped by
`<folder>/<folder>.md`. Dependencies point down this list and never back up; `cmake/voe.cmake`
holds the allowed edges, so a change needing a new edge is a decision, not a card. The standing
rules are decision 0168.

- **base** — arenas, containers, strings, the two asserts, error codes, described structs.
- **math** — vectors and matrices spelled the way Slang spells them; knows no graphics API.
- **ecs** — entities, component tables, intent queues, a structural queue, a type's menu path.
- **platform** — the one OS-aware folder: window, input, keymap, files, clock, processes, libraries, arguments.
- **scene** — transform, a camera's lens, light and identity as components with their systems.
- **assets** — glTF, images, fonts and the sectioned text format to CPU data; the JSON parser.
- **authoring** — scene and project text read and written, a world cooked to C; a game omits it.
- **render** — the GPU layer and the only folder that names Vulkan: the card, resources, passes.
- **text** — a glyph atlas as a distance field and one mesh per text block; Oxanium, the one face.
- **ui** — immediate-mode GUI in millimetres: layout, panels, widgets, overlays, one draw.
- **theme** — a `.theme` file's bytes read into `ui`'s authored inputs; opens no file.
- **3d** — scene → draws → a render target: meshes, shapes, picking, outlines, gizmo, camera marker.
- **sprite** — a sprite is a plane in the world and hands back a material.
- **app** — what a frame loop repeats: the paced frame, capture to a PNG, a headless start.
- **game** — the loop a shipped game runs: its world's types, a frame, the run, the code seam.
- **editor** — authoring: top bar, Scene list, views to fly, gizmo, Inspector, undo, Play, Refresh.
- **dev** — the program that shows what the engine can do; a leaf on all but editor and authoring.
- **testing** — the check macros a test links as `voe::testing`; not a library, not in the map.
- **examples** — example projects as data, one folder each, no target; `capsule/capsule.md`.

## Decisions in force

0001–0167 stand in `history/decisions/`; 0168 on are indexed in `decisions/decisions.md`.

- 0168 — The Agentic workflow replaces specs; the engine's rules are one digest. On every card.
- 0001, 0005, 0008, 0026, 0047 — One repo of static CMake folders, no plugin; Clang 19+ as `clang`.
- 0004, 0021, 0023, 0028, 0031, 0042, 0213 — Tools installed, deps fetched; no CI; CTest.
- 0007, 0011, 0013, 0017 — The scene is an ECS; a component and its system are one module.
- 0014, 0029, 0032, 0034, 0041 — Naming carries the namespace; memory is arenas; failure is a value.
- 0022, 0121, 0135, 0151 — The folder map above; editor and dev are leaves; a game omits authoring.
- 0033, 0035 — Right-handed +Y up −Z forward, reversed depth, one Y flip; `math` speaks Slang.
- 0015, 0040, 0046 — Vulkan without an SDK; shaders in Slang, compiled at build time and embedded.
- 0113, 0130 — A change owns the call sites it breaks downstream; Linux alone verifies a card.
- 0120, 0133, 0173 — Every code folder carries a `.md`; `src/` and `tests/` list their own files.
- 0169–0172, 0178 — A key has four levels; a theme is read in `theme`, derived in `ui` in OKLab.
- 0174–0177, 0208–0211 — A new folder registers itself; draws as PNGs; 006, 015 and 016 closed.
- 0180–0185, 0212 — Wayland at fractional scale; editor text scales, hard-edged, dilated when thin.
- 0186–0188, 0234–0237 — Toward a coin game in C; Play builds a `game` tree; our formats are text.
- 0189, 0190, 0191 — An entity is a number; the world owns its rows; a shape has a colour.
- 0194, 0196, 0231, 0232 — One `hue=`; roles differ in lightness; state and reached borders invert.
- 0197, 0219, 0220, 0224–0226, 0228, 0229 — Text scale by theme; fit; panels in mm, views a share.
- 0192, 0193, 0195, 0198–0200 — Structure via the queue; one focus; dropdowns; overlays fit.
- 0201, 0214 — The fastest card is the best kind then the largest memory; one line says which.
- 0202–0207 — Pick by ray; outline quads; undo is scene texts; the gizmo is `3d`'s.
- 0215, 0216, 0227, 0233 — Unfocused, 4 fps; hidden, none; a pointer shape; right button flies.
- 0217, 0218, 0221 — An entity, then components offered by each type's registered path; one camera.
- 0222, 0223, 0238 — Placed by transform; the camera is a lens and a marker; no light draws unlit.
- 0239–0243, 0245 — A project's logic is its C in `Code/`, a library the editor loads and Refreshes.
- 0244, 0246–0248 — Examples in root `examples/`, checked on structure; every string is UTF-8, Windows too.
