# System

The map of voe3d (Voluntary Overtime Engine 3D), a real-time 3D engine in C23 on Vulkan 1.3, its
editor and dev program in one tree. Every code folder is a standalone CMake project —
`include/<folder>/`, `src/`, `tests/`, a four-line `CMakeLists.txt` — built as `voe_<folder>`,
linked as `voe::<folder>`, mapped by `<folder>/<folder>.md`. Dependencies point down this list and
never back up; `cmake/voe.cmake` holds the allowed edges, so a change needing a new edge is a
decision, not a card. The standing rules are decision 0168, named on every card.

- **base** — arenas, containers, strings, the two asserts, error codes, described structs.
- **math** — vectors and matrices spelled as Slang spells them; double3 for a world position.
- **ecs** — entities, tables, intent and structural queues; a type's menu, unsaid row, former names.
- **platform** — the one OS-aware folder: window, input, pads, files, clock, processes, libs, sound.
- **scene** — transform (double, relative, a step behind), parent, prefab, lens, lights, blockers.
- **physics** — colliders, overlap and sweep queries, a kinematic body's move; `physics/physics.md`.
- **assets** — glTF, images, WAV, fonts and sectioned text to CPU data; the JSON parser.
- **audio** — the mixer: voices by handle, looped, pitched, placed by the camera, paused; sound row.
- **authoring** — scene, prefab and project text (game window too) read and written; cooked to C.
- **render** — all of Vulkan: card, resources, mip chains, passes, light, bounce, a pipeline cache.
- **text** — a distance-field glyph atlas, Latin-1 and a few symbols; one mesh per block; Oxanium.
- **ui** — immediate-mode GUI in millimetres: layout, panels, widgets, overlays, one draw.
- **theme** — a `.theme` file's bytes read into `ui`'s authored inputs; opens no file.
- **3d** — scene to draws: meshes, models, shapes, particles, water, picks, markers, blocked light.
- **sprite** — a sprite is a plane in the world and hands back a material.
- **app** — a frame loop's parts: paced frame, PNGs, headless start, start log, cache file.
- **game** — a shipped game in its window: world types, 1/60 s steps, code, sound, splash worker.
- **editor** — top bar, Project, Scene list, Assets, prefabs, views, gizmo, Inspector, Play, GPU ms.
- **dev** — the program that shows what the engine can do; a leaf on all but editor and authoring.
- **testing** — the check macros a test links as `voe::testing`; not a library, not in the map.
- **examples** — example projects as data, one folder each, no target; each has its own `.md`.
- **engine_assets** — the human's logo and input material; not code; agents read, link, never write.

## Decisions in force

0001–0167 stand in `history/decisions/`; 0168 on are indexed in `decisions/decisions.md`.
- 0001, 0005, 0008, 0026, 0047 — One repo of static CMake folders, no plugin; Clang 19+ as `clang`.
- 0004, 0021, 0023, 0028, 0031, 0042, 0213 — Tools installed, deps fetched; no CI; CTest.
- 0007, 0011, 0013, 0017 — The scene is an ECS; a component and its system are one module.
- 0014, 0029, 0032, 0034, 0041 — Naming carries the namespace; memory is arenas; failure is a value.
- 0022, 0121, 0135, 0151 — The folder map above; editor and dev are leaves; a game omits authoring.
- 0033, 0035, 0250 — +Y up −Z forward, reversed depth; double positions, camera-relative on the GPU.
- 0015, 0040, 0046 — Vulkan without an SDK; shaders in Slang, compiled at build time and embedded.
- 0113, 0120, 0130, 0133, 0173 — A change owns what it breaks; Linux verifies; a folder has a `.md`.
- 0169–0172, 0178 — A key has four levels; a theme is read in `theme`, derived in `ui` in OKLab.
- 0174–0177, 0208–0211 — A new folder registers itself; draws as PNGs; 006, 015 and 016 closed.
- 0180–0185, 0212, 0269 — Wayland at fractional scale; text scales, dilated when thin, edge ramped.
- 0186–0188, 0234–0237, 0251–0252, 0258–0262, 0264–0266 — Coin game; shadows; Play; UI; Ship; sound.
- 0194, 0196, 0231, 0232 — One `hue=`; roles differ in lightness; state and reached borders invert.
- 0197, 0219, 0220, 0224–0226, 0228, 0229, 0306, 0344 — Text scale; fit; mm panels; PC sizes.
- 0192, 0193, 0195, 0198–0200, 0202–0207 — Queue; focus; dropdowns; overlays; picks; undo; gizmo.
- 0189–0191, 0201, 0214–0218, 0221, 0227, 0233, 0300, 0302, 0303 — Fastest card; 4 fps; one camera.
- 0222, 0223, 0273–0276, 0287–0290 — Placed by transform; markers; rings; no light: preview, black.
- 0239–0248, 0263 — A project's C in `Code/`, loaded, Refreshed; `examples/`; `engine_assets/`.
- 0249, 0253–0257, 0293 — Colliders, overlap, sweep, kinematic body; 60 steps a second; two slots.
- 0267, 0268, 0270–0272, 0277–0286, 0291, 0292, 0294–0297 — 0.1 closed; tank game; models; hits.
- 0298, 0299, 0301, 0304, 0305, 0321, 0324, 0325 — Particles; effects; shadows; sounds; water; lamps
- 0307, 0312, 0316–0319, 0328, 0331 — Bounce per light, on the level; not screen space; opt-in.
- 0333–0340, 0342, 0343, 0346 — Pause; waves; ids; fade; Linux; Release; copy; browse; splash.
- 0348, 0349, 0351–0355, 0361, 0363, 0364 — Block kinds; sun, moon; fields; panels closed; √.
- 0358, 0359, 0362, 0368, 0370 — GPU timed per pass; Best Practices; far mips; splash; Bistro.
