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
10. **Implement on demand. No "in case".** A function is written when something
    calls it; a type when something stores it. The set is not completed for
    symmetry — `float3` without `float2` is correct if nothing has a `float2`.
    A card specifying surface nothing uses is over-specified: report it, the
    same as one that is missing something.
11. **Memory is an arena you are handed.** Working memory comes from a
    `voe_base_arena` passed in as a parameter, and is freed by rewinding or
    destroying the arena — never one allocation at a time. There is no global
    or default arena. Long-lived or growable data is the exception and uses
    `new`/`destroy` instead. **Failing to allocate is fatal, not a returned
    `NULL`.** Do not write a null check after an allocation.
12. **Tests are plain C programs, one per module.** `<folder>/tests/<module>.c`,
    an ordinary `main()`, zero for pass. The build finds them; nothing is
    registered anywhere. Check macros come from `voe::testing` — link it from
    tests only, never from `src/`. A failure message names the expression, the
    expected value and the actual one, because that message is all the next
    reader gets.

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

## Conventions the world is built on

Silent when wrong. Where these disagree with a reference you are copying from,
they win and the reference is wrong for this engine.

- **Right-handed, +Y up, −Z forward.** A camera looks along its own −Z. This is
  glTF's convention, so the importer converts **no** coordinates — and may not
  be given a conversion later. Layout is a different matter: see below.
- **Vectors are columns.** `M · v`; composition reads right to left; translation
  is the last column.
- **Matrices are stored row-major — `m[row][column]`** — because that is what
  the same expression means in Slang, so an upload to the GPU is a straight
  copy. `slangc` must be invoked with `-matrix-layout-row-major`; its default is
  the other one and getting it wrong transposes every transform without failing
  to compile. glTF stores column-major, so the importer transposes — layout
  only, never coordinates.
- **`math` is spelled the way Slang spells it.** `voe_math_float3`,
  `voe_math_float4x4`. Operation names follow Slang's operators, not
  mathematical vocabulary: `_mul` is component-wise `a * b`, **not** a dot
  product; scalar multiplication is `_scale`.
- **Depth runs backwards, deliberately.** The near plane is **1.0**, the far
  plane **0.0**, range 0..1, float depth buffer, `GREATER`, cleared to 0 —
  orthographic included. Nearly every tutorial does the opposite. Do not
  "correct" it.
- **Exactly one Y flip exists in the engine, and it is in the viewport.**
  Vulkan's clip space is Y-down; that is reconciled with a negative viewport
  height in `render`, never by negating a row of a projection matrix. Flipping
  Y reverses apparent winding, so the front-face constant is set to match and
  proven by a test. Flipping twice is the classic bug and it is invisible until
  culling is on.
- **`math` does not know about any graphics API** and does not build projection
  matrices.
- Angles are radians. Units are metres and seconds.

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

**The build works on Linux and Windows.** `cmake/voe.cmake`, `check.cmake` and
the folder scaffolding are in place and card 001 is complete. `math` is the
first real code and is in progress.

Settled and binding: naming, the build, tests, memory, and the conventions
above. **Error handling is not decided** — how a genuine, recoverable failure is
reported (a file that will not open, a model that will not parse) is still open,
and a card that needs it says so rather than inventing one. Note that running
out of memory is *not* one of these: that is fatal, and decided.

Throwaway spikes do not live in this repository.
