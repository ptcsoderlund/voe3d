# VOE3D

A general-purpose real-time 3D engine. This repository is for **executing and
writing**. It is self-contained — nothing here depends on a parent directory.

## Read first

| Document | Authority over |
|---|---|
| `guidelines.md` | Scope of work, folder structure, `<folder>.md` files, coupling, workflow, design preference. |
| `coding_convention.md` | Code style, naming, modules, allocation. |

Both are authoritative. This file does not restate them.

`guidelines.md` uses C#, C++20 and F# only as examples. Every rule in it applies
to C. Read it with this mapping:

| In `guidelines.md` | In C |
|---|---|
| namespace | a **folder** — the folder *is* the namespace |
| module (a language construct) | a **component and its system** — see `coding_convention.md`. It may span several files in one folder, all sharing the module name. |
| class owning its data | a component struct plus its system; fields are not written from outside that module |

## Givens

| | |
|---|---|
| Language | C23 (plain C — not C++) |
| Compiler | **Clang 18+ only, as the GNU-driver `clang` on both platforms.** Not `clang-cl`, not MSVC, not GCC — including GCC on Linux. CMake fails configuration for anything else |
| Graphics | Vulkan 1.3 only |
| Shaders | Slang, compiled to SPIR-V |
| Platforms | Windows and Linux desktop only |
| Build | CMake 3.28+, Ninja generator, both platforms |

**Required tools, installed by the programmer:** Clang 18+ (the standalone LLVM
release is the recommended install on Windows), CMake, `slangc`. On Windows
also the Windows SDK and MSVC C runtime from Visual Studio or its Build Tools;
`clang.exe` finds them itself.

**Onboarding invariant:** a programmer clones this repository and builds without
assembling an environment. The line:

> Tools that transform source are installed by the programmer.
> Anything the engine links against or ships is fetched by the build.

An option that breaks this is rejected on that ground alone.

## Rules

1. **Every folder configures standalone.** A folder is the unit CMake links. It
   owns a four-line `CMakeLists.txt` (see *Build*), keeps its public headers in
   `include/<folder>/` and its sources in `src/`, and never reaches into a
   sibling's source directory.
2. **Folder dependencies point one way.** No upward or sideways dependency, no
   cycles. The allowed edges are data in `cmake/voe.cmake`; a `DEPENDS` outside
   them fails configuration. A change that needs one is reported, not made.
3. **Data is read by anyone, written by one.** Any code may read any component,
   read-only or by copy. A component is written only by its own system.
4. **To change another module's data, submit an intent.** An intent is a
   datatype, handed to the ECS, drained by the owning system. **Never call
   another system.** No `system -> system` dependencies.
5. **Write it ourselves.** Third-party dependencies are not added without
   asking first. The default answer is to write it.
6. **No `**`.** One level of dereference. Not to be worked around with a typedef.
7. **The prefix is `voe_`, everywhere.** Functions, types, CMake targets and
   projects. The namespace is spelled out: `voe_math_vec3_add`, `voe_render_…`.
8. **No CI. `cmake -P check.cmake` is the verification.** It exits zero on the
   coder's machine before a card moves to `review/`, and on the human's before
   it moves to `complete/`.
9. **Do not push or pull.** Remote git operations are the human's.

## Folders

Dependencies point down. Nothing points back up.

```
app                     the frame loop, wiring
├── 3d                  the 3D renderer: scene → draws → a render target
│   ├── render          Vulkan. The only folder that names it
│   │   └── platform    window, input, files, time. The only OS-aware folder
│   ├── assets          glTF, images, fonts → CPU data
│   └── scene           transform, camera, light
│       └── ecs         entities, components, intent queues
├── text · sprite · ui  planned, not yet created — each on render, low level
└── base · math         memory, containers, strings, assert · vectors, matrices
```

| Folder | Depends on |
|---|---|
| `base`, `math` | — |
| `ecs` | `base` |
| `platform` | `base` |
| `scene` | `ecs`, `math`, `base` |
| `assets` | `platform`, `math`, `base` |
| `render` | `platform`, `math`, `base` |
| `3d` | `render`, `scene`, `ecs`, `assets`, `math`, `base` |
| `app` | all |

`render` is the GPU layer only — device, memory, resources by id, pipelines,
render targets, present. It knows nothing about scenes, entities or files; a gap
in its API gets a card in `render`, never a reach-around from `3d`. Renderers
turn already-parsed data into draws; no renderer parses a document format.
`math` is pure data with support functions and has no systems. Nothing renders
to the screen directly: the frame is drawn to a target, and presenting it is a
separate last step.

## Build

Every folder's `CMakeLists.txt` is these four lines and nothing else:

```cmake
cmake_minimum_required(VERSION 3.28)
project(voe_scene C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(scene DEPENDS ecs math base)
```

`voe_module()` does the rest: compiler guards, the one flag set, dependencies,
the target `voe_<folder>` and its alias `voe::<folder>` (link the alias), and
globbing `src/`. **Adding a source file needs no CMake edit.** The `DEPENDS`
line is the folder's row in the table above, and it is checked.

Daily work: `cmake --preset debug` at the root, then build `voe_<folder>`.
Standalone configuration is a property `check.cmake` proves, not a workflow.

## Work

`kanban/todo/` → `review/` → `complete/`. One card per file, `NNN-slug.md`,
shaped by `kanban/CARD-TEMPLATE.md`. Set `claimed-by:` before starting — there
is no in-progress bucket, so that field is the only claim signal.

Cards arrive in `todo/` already decided. Implement what the card says; a card
that is unclear is asked about, not guessed at. `cmake -P check.cmake` exits
zero before the card moves to `review/`.

## Status

**No engine source or build files yet.** Architecture, naming and the build
layout are settled; the first scaffolding cards — `cmake/voe.cmake`,
`check.cmake`, then `base` and `math` — are being written. Error handling,
memory allocation and the test mechanism are not yet decided; a card that needs
one of them says so. Throwaway spikes do not live in this repository.
