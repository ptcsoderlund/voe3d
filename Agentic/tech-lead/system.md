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
- **platform** — the one OS-aware folder: window, wait, input, pointer shape, keymap, files, clock.
- **scene** — transform, a camera's lens, light and identity as components with their systems.
- **assets** — glTF, images, fonts and the sectioned text format to CPU data; the JSON parser.
- **authoring** — scene and project text read and written; a game does not build it.
- **render** — the GPU layer and the only folder that names Vulkan: the card, resources, passes.
- **text** — a glyph atlas as a distance field and one mesh per text block; Oxanium, the one face.
- **ui** — immediate-mode GUI in millimetres: layout, panels, widgets, overlays, one draw.
- **theme** — a `.theme` file's bytes read into `ui`'s authored inputs; opens no file.
- **3d** — scene → draws → a render target: meshes, shapes, picking, outlines, gizmo, camera marker.
- **sprite** — a sprite is a plane in the world and hands back a material.
- **app** — what a frame loop repeats: the paced frame, capture to a PNG, a headless start.
- **editor** — authoring: top bar, Scene list, views to fly, gizmo, Inspector, undo, sizes, files.
- **dev** — the program that shows what the engine can do; a leaf on all but editor and authoring.
- **testing** — the check macros a test links as `voe::testing`; not a library, not in the map.

## Decisions in force

0001–0167 stand in `history/decisions/`; 0168 on are indexed in `decisions/decisions.md`.

- 0168 — The Agentic workflow replaces specs; the engine's rules are one digest. On every card.
- 0001, 0008 — One repository of self-contained CMake folders, statically linked; no plugin.
- 0005, 0026, 0047 — Clang only, as the GNU-driver `clang` on both platforms, floor 19.
- 0021, 0023 — Tools installed by the programmer, dependencies fetched by the build.
- 0004, 0028, 0031, 0042, 0213 — No CI; the Checks run in the tree; tests are plain C under CTest.
- 0007, 0011, 0013, 0017 — The scene is an ECS; a component and its system are one module.
- 0014, 0029, 0032, 0034, 0041 — Naming carries the namespace; memory is arenas; failure is a value.
- 0022, 0121, 0135, 0151 — The folder map above; editor and dev are leaves; a game omits authoring.
- 0033, 0035 — Right-handed +Y up −Z forward, reversed depth, one Y flip; `math` speaks Slang.
- 0015, 0040, 0046 — Vulkan without an SDK; shaders in Slang, compiled at build time and embedded.
- 0113, 0130 — A change owns the call sites it breaks downstream; Linux alone verifies a card.
- 0120, 0133, 0173 — Every code folder carries a `.md`; `src/` and `tests/` list their own files.
- 0169–0172, 0178 — A key has four levels; a theme is read in `theme`, derived in `ui` in OKLab.
- 0174–0177 — 006 merged back; a new folder registers itself; `theme` sees `render`; draws as PNGs.
- 0180–0185, 0212 — Wayland at fractional scale; editor text scales, hard-edged, dilated when thin.
- 0186, 0187, 0188, 0234 — Toward a coin game in the project's own C; Play cooks the scene as it is; Play becomes Stop.
- 0189, 0190, 0191 — An entity is a number; the world owns its rows; a shape has a colour.
- 0193, 0195, 0198 — The editor changes structure through the queue; a named field is a dropdown.
- 0194, 0196 — One hue authored as `hue=`, roles differing only in lightness, state drawn inverted.
- 0197, 0219, 0220, 0224–0226, 0228, 0229 — Text scale by theme; fit; panels in mm, views a share.
- 0231, 0232 — A draggable border rests in the border colour, reached drawn as the inverse pair.
- 0192, 0199, 0200 — `ui` holds one focus; an overlay belongs to its widget and fits.
- 0201, 0214 — The fastest card is the best kind then the largest memory; one line says which.
- 0202–0204 — A click picks by a ray on the shapes' triangles; outline quads; undo is scene texts.
- 0205, 0206, 0207 — The move gizmo is `3d`'s, its drag the editor's; state is lightness and size.
- 0208–0211 — 015 closes on its walk; 016 moves comments only, and a page's opening says what for.
- 0215, 0216, 0227, 0233 — Unfocused, 4 fps; hidden, none; a pointer shape; right button flies.
- 0217, 0218, 0221 — An entity, then components offered by each type's registered path; one camera.
- 0222, 0223 — Anything in 3D space is placed by its transform; the camera is a lens and a marker.
