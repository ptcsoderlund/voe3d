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
| Compiler | **Clang 19+ only, as the GNU-driver `clang` on both platforms.** Not `clang-cl`, not MSVC, not GCC — including GCC on Linux. CMake fails configuration for anything else. 19 and not 18 because a compiled shader reaches the binary through `#embed` |
| Graphics | Vulkan 1.3 only |
| Shaders | Slang, compiled to SPIR-V by `slangc` at build time and `#embed`ded in the binary. Nothing is read from disk at run time |
| Platforms | Windows and Linux desktop only |
| Build | CMake 3.28+, Ninja generator, both platforms |

**Required tools, installed by the programmer.** `check.cmake` step 1 checks
every one of these and names the one that is missing, so a fresh clone fails
with an answer rather than a compiler error:

| Tool | Both | Windows only | Linux only |
|---|---|---|---|
| Clang 19+, GNU driver (standalone LLVM release recommended on Windows) | ● | | |
| CMake 3.28+ | ● | | |
| Ninja | ● | | |
| `slangc` | ● | | |
| Windows SDK + MSVC C runtime (Visual Studio or its Build Tools) — `clang.exe` finds them itself | | ● | |
| Wayland development package, providing `wayland-scanner` and the client library | | | ● |

**Optional, and the build never requires them:** the Vulkan SDK — it is the
easiest way to get `slangc`, and it brings the validation layers, which the
engine uses when they are present and runs without when they are not. And
`clang-tidy`, which nothing invokes: `check.cmake` step 7 uses `clang --analyze`,
which is part of the compiler already required. Neither is on the list above and
neither may become a build requirement.

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
   it moves to `complete/`. That includes step 6 (tests) and step 7
   (`clang --analyze`) — an analyser warning fails the script exactly as a
   warning does, and there is no baseline file and no tolerated count. Where the
   analyser is genuinely wrong — the arena never frees, which a leak checker is
   built to complain about — suppress it **at that site with a comment saying
   why**. Never globally, and never by changing correct code to quiet it.
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
13. **A failure the world caused is returned. A failure the program caused is an
    assert.** A bad parameter — a `NULL` where the API requires a thing — is the
    caller's bug: `VOE_BASE_ASSERT`. A file that will not open, a device that
    will not create, a compositor that is not there: returned to the caller, who
    decides.

    **One way to fail** → return `NULL` (from a `_new`) or `false`. Nothing
    else; do not add an error parameter with one value in it.
    **Several distinguishable ways** → still return `NULL`, and take one
    `voe_base_error *error` out-parameter beside it. Where there is nothing to
    return, the enum *is* the return value and `VOE_BASE_OK` is `0`.

    ```c
    [[nodiscard]] voe_platform_window *voe_platform_window_new(int w, int h,
                                                               const char *title);
    [[nodiscard]] voe_render_device *voe_render_device_new(voe_platform_native n,
                                                           voe_base_error *error);
    ```

    **`[[nodiscard]]` on every function that can fail**, no exceptions — with
    `-Werror` that makes an unchecked failure a build error rather than
    something review has to catch. `error` may be `NULL` when the caller has
    decided it does not need to know which way. The codes live in one enum in
    `base`, are **added when a card needs one** and never in advance, and are
    **categories, not incidents** — which of them it was goes in the enum, what
    exactly happened goes in the message at the site.
14. **Do not recurse over data read from a file.** Recursive descent on a glTF
    or JSON file is a stack overflow on deep nesting, and it is a real crash on
    a corrupt or hostile asset that neither `-Werror` nor the analyser will find.
    Explicit stack, a named nesting limit, refuse the file past it, reason in the
    file header. This binds `assets`; recursion over data the engine built itself
    is fine.

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

**One platform is enough to finish a card (2026-09-04).** There is one machine
and one operating system on it at a time, so a card is not held in `review/`
waiting for the other platform. `check.cmake` green on the platform in front of
you, and the card moves to `complete/`; the other platform is checked whenever
that machine is next booted, and what it turns up becomes a new card rather than
reopening the old one. A card's notes say which platform it was verified on and
what could not be checked there, because that note is what the bug report is
written against later.

## Status

**The build works on Linux and Windows.** `cmake/voe.cmake`, `check.cmake` and
the folder scaffolding are in place and card 001 is complete. `math` is the
first real code and is in progress.

Settled and binding: naming, the build, tests, memory, error handling, and the
conventions above. **Error handling is now decided — rule 13.** Running out of
memory is not one of those failures: that is fatal, through rule 11.

**Vulkan needs no SDK, and there is nothing to install for it.** The headers are
vendored into the tree, declarations only; the loader (`vulkan-1.dll`,
`libvulkan.so.1`) ships with the graphics driver and is opened by name at
startup, with every entry point resolved through `vkGetInstanceProcAddr`. There
is no `-l` flag, no import library, and nothing fetched. **So every Vulkan call
goes through a resolved function pointer** — that table is `render`'s alone,
resolved once at startup, never reassigned, never passed as a parameter and
never stored in a component. It is the only place in this engine where function
pointers are expected, and its file header must say so. A missing loader is a
returned failure under rule 13, not a crash.

Throwaway spikes do not live in this repository.
