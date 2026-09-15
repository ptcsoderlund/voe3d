# VOE3D

A general-purpose real-time 3D engine in C23 and Vulkan, with its editor and dev program built from the same tree. Work on it through `/product`.

## Checks

- Build: `cmake --preset debug`, then `cmake --build --preset debug --target voe_<folder>`
- Check: `cmake -P check.cmake` — tools, every folder configured standalone, the root build, the guards, includes, tests, `clang --analyze`
- Test one folder: `ctest --test-dir build/debug -R '^<folder>/'` — tests are registered as `<folder>/<module>`, so build first

## Conventions

- The guidelines skill and its `lang-c.md` apply. What follows is this project's and wins where it is stricter.
- C prefix: `voe_`, everywhere (rule 7).
- Tests live beside the code, at `<folder>/tests/<module>.c` (rule 12).
- Reading the guidelines in C: a namespace is a **folder**; a module is a **component plus its system**, possibly several files in one folder sharing the module name; a class owning its data is a component struct whose fields only its own system writes.
- `ADR-NNNN` in code and comments means `Agentic/decisions/NNNN-*.md`. "card NNN" means a file under `history/cards/`, "bug NNN" one under `history/bugs/`.
- Throwaway spikes do not live in this repository.

## Givens

| | |
|---|---|
| Language | C23 (plain C — not C++) |
| Compiler | **Clang 19+ only, as the GNU-driver `clang` on both platforms.** Not `clang-cl`, not MSVC, not GCC — GCC on Linux included. Configuration fails for anything else. 19 because a compiled shader reaches the binary through `#embed` |
| Graphics | Vulkan 1.3 only |
| Shaders | Slang, compiled to SPIR-V by `slangc` at build time and `#embed`ded. Nothing is read from disk at run time |
| Platforms | Windows and Linux desktop only |
| Build | CMake 3.28+, Ninja generator, both platforms |

**Required tools, installed by the programmer.** `check.cmake` step 1 checks each and names the one missing:

| Tool | Both | Windows only | Linux only |
|---|---|---|---|
| Clang 19+, GNU driver (standalone LLVM release recommended on Windows) | ● | | |
| CMake 3.28+ | ● | | |
| Ninja | ● | | |
| `slangc` | ● | | |
| Windows SDK + MSVC C runtime (Visual Studio or its Build Tools) — `clang.exe` finds them itself | | ● | |
| Wayland development package, providing `wayland-scanner` and the client library | | | ● |

**Optional, never required by the build:** the Vulkan SDK (the easy way to get `slangc`; brings validation layers, used when present) and `clang-tidy` (nothing invokes it; step 7 uses `clang --analyze`). Neither may become a build requirement.

**Onboarding invariant:** a programmer clones and builds without assembling an environment.

> Tools that transform source are installed by the programmer.
> Anything the engine links against or ships is fetched by the build.

An option that breaks this is rejected on that ground alone.

## Rules

1. **Every folder configures standalone.** A folder is the unit CMake links. It owns a four-line `CMakeLists.txt` (see *Build*), keeps public headers in `include/<folder>/` and sources in `src/`, and never reaches into a sibling's source directory.
2. **Folder dependencies point one way.** No upward or sideways dependency, no cycles. The allowed edges are data in `cmake/voe.cmake`; a `DEPENDS` outside them fails configuration. A change that needs a new edge is reported, not made.
3. **Data is read by anyone, written by one.** Any code may read any component, read-only or by copy. A component is written only by its own system. The exception is creation: a folder's typed creation call and `authoring`'s scene reader add rows to entities they have just made, and neither edits one (ADR-0152).
4. **To change another module's data, submit an intent.** An intent is a datatype, handed to the ECS, drained by the owning system. **Never call another system.** No `system -> system` dependencies.
5. **Write it ourselves.** No third-party dependency is added without asking the sponsor first. The default answer is to write it.
6. **No `**`.** One level of dereference. Not to be worked around with a typedef.
7. **The prefix is `voe_`, everywhere.** Functions, types, CMake targets and projects. The namespace is spelled out: `voe_math_float3_add`, `voe_render_…`.
8. **No CI. `cmake -P check.cmake` is the verification, and it exits zero before a task is done.** That includes step 6 (tests) and step 7 (`clang --analyze`): an analyser finding fails the script exactly as a warning does. There is no baseline file and no tolerated count. Where the analyser is genuinely wrong, suppress **at that site with a comment saying why** (the form is in `check.cmake` step 7's header). Never globally, and never by changing correct code to quiet it.
9. **Do not push or pull.** Remote git operations are the human's.
10. **Implement on demand. No "in case".** A function is written when something calls it; a type when something stores it. A set is not completed for symmetry — `float3` without `float2` is correct if nothing has a `float2`. A spec or task specifying surface nothing uses is over-specified: report it, as you would one missing something.
11. **Memory is an arena you are handed.** Working memory comes from a `voe_base_arena` passed as a parameter and is freed by rewinding or destroying the arena — never one allocation at a time. There is no global or default arena. Long-lived or growable data is the exception and uses `new`/`destroy`. **Failing to allocate is fatal, not a returned `NULL`.** No null check after an allocation.
12. **Tests are plain C programs, one per module.** `<folder>/tests/<module>.c`, an ordinary `main()`, zero for pass. The build finds them; nothing is registered. Check macros come from `voe::testing` — linked from tests only, never from `src/`. A failure message names the expression, the expected value and the actual one.
13. **A failure the world caused is returned. A failure the program caused is an assert.** A bad parameter — a `NULL` where the API requires a thing — is the caller's bug: `VOE_BASE_ASSERT`. A file that will not open, a device that will not create, a compositor that is not there: returned to the caller, who decides.
    - **One way to fail** → return `NULL` (from a `_new`) or `false`. No error parameter with one value in it.
    - **Several distinguishable ways** → still return `NULL`, and take one `voe_base_error *error` out-parameter beside it. Where there is nothing to return, the enum *is* the return value and `VOE_BASE_OK` is `0`.
    - **`[[nodiscard]]` on every function that can fail**, no exceptions; with `-Werror` an unchecked failure is a build error. `error` may be `NULL` when the caller does not need to know which way.
    - The codes live in one enum in `base`, are **added when code needs one**, never in advance, and are **categories, not incidents** — which one goes in the enum, what exactly happened goes in the message at the site.

    ```c
    [[nodiscard]] voe_platform_window *voe_platform_window_new(int w, int h, const char *title);
    [[nodiscard]] voe_render_device *voe_render_device_new(voe_platform_native n, voe_base_error *error);
    ```
14. **Do not recurse over data read from a file.** Recursive descent on a glTF or JSON file is a stack overflow on deep nesting — a real crash on a corrupt or hostile asset that neither `-Werror` nor the analyser finds. Explicit stack, a named nesting limit, refuse the file past it, reason in the file header. This binds `assets`; recursion over data the engine built itself is fine.

## Scope of a task

- **A task that changes its folder's public surface also updates the call sites that change breaks, in folders downstream of it** (ADR-0113). This overrides the guidelines' `BLOCKED` rule for that case only. Downstream is checkable: the edges in `cmake/voe.cmake`. **No new edge, no reversed arrow, no reaching into a sibling's source** — those stay `BLOCKED: <folder>, <why>`. It covers what the change broke and nothing else: *would the tree build without this edit?* If yes, the edit is not covered. Name the call sites touched in the report.
- **One platform is enough, Linux first** (ADR-0130). `check.cmake` green on Linux finishes a task; nothing waits for Windows. Windows code is still written and `platform` keeps both backends as equals. What the other platform turns up is reported, not reopened. Say which platform a task was verified on and what could not be checked there.

## Folders

Dependencies point down. Nothing points back up.

```
dev · editor            programs a person runs; leaves, nothing names them (ADR-0121)
app                     parts a program calls in its own frame loop (ADR-0135)
├── 3d                  the 3D renderer: scene → draws → a render target
│   ├── render          Vulkan. The only folder that names it
│   │   └── platform    window, input, files, time. The only OS-aware folder
│   ├── assets          glTF, images, fonts → CPU data
│   └── scene           transform, camera, light
│       └── ecs         entities, components, intent queues
├── authoring           world ↔ scene text; not linked by a game
├── text · ui           on render, low level
├── sprite              on 3d, not beside text: it hands back a material
└── base · math         memory, containers, strings, assert · vectors, matrices
```

| Folder | Depends on |
|---|---|
| `base`, `math` | — |
| `ecs` | `base` |
| `platform` | `base` |
| `scene` | `ecs`, `math`, `base` |
| `assets` | `platform`, `math`, `base` |
| `authoring` | `scene`, `ecs`, `assets`, `math`, `base` |
| `render` | `platform`, `math`, `base` |
| `text` | `render`, `math`, `base` |
| `ui` | `render`, `text`, `math`, `base` |
| `3d` | `render`, `scene`, `ecs`, `assets`, `math`, `base` |
| `sprite` | `3d`, `render`, `math`, `base` |
| `app` | `3d`, `render`, `assets`, `platform`, `scene`, `ecs`, `math`, `base` |
| `editor` | `app`, `authoring`, `3d`, `ui`, `text`, `render`, `platform`, `scene`, `ecs`, `math`, `base` |
| `dev` | every folder but `editor` and `authoring` |

`cmake/voe.cmake` is the authority; this table is its copy.

`render` is the GPU layer only — device, memory, resources by id, pipelines, render targets, present. It knows nothing about scenes, entities or files; a gap in its API is work in `render`, never a reach-around from `3d`. Renderers turn already-parsed data into draws; no renderer parses a document format. `math` is pure data with support functions and has no systems. Nothing renders to the screen directly: the frame is drawn to a target, and presenting it is a separate last step.

**Vulkan needs no SDK.** The headers are vendored in `render/vulkan/`, declarations only; the loader (`vulkan-1.dll`, `libvulkan.so.1`) ships with the driver and is opened by name at startup, every entry point resolved through `vkGetInstanceProcAddr`. No `-l` flag, no import library, nothing fetched. **So every Vulkan call goes through `render`'s resolved function-pointer table** — resolved once at startup, never reassigned, never passed as a parameter, never stored in a component. It is the only place function pointers are expected, and its file header says so. A missing loader is a returned failure under rule 13, not a crash.

## Conventions the world is built on

Silent when wrong. Where a reference disagrees, the reference is wrong for this engine.

- **Right-handed, +Y up, −Z forward.** A camera looks along its own −Z. This is glTF's convention, so the importer converts **no** coordinates, and may not be given a conversion later.
- **Vectors are columns.** `M · v`; composition reads right to left; translation is the last column.
- **Matrices are stored row-major — `m[row][column]`** — because that is what the same expression means in Slang, so an upload is a straight copy. `slangc` runs with `-matrix-layout-row-major`; its default transposes every transform without failing to compile. glTF stores column-major, so the importer transposes — layout only, never coordinates.
- **`math` is spelled the way Slang spells it.** `voe_math_float3`, `voe_math_float4x4`. Operation names follow Slang's operators: `_mul` is component-wise `a * b`, **not** a dot product; scalar multiplication is `_scale`.
- **Depth runs backwards, deliberately.** Near plane **1.0**, far plane **0.0**, range 0..1, float depth buffer, `GREATER`, cleared to 0 — orthographic included. Do not "correct" it.
- **Exactly one Y flip exists, and it is in the viewport.** Vulkan's clip space is Y-down; that is reconciled with a negative viewport height in `render`, never by negating a projection row. The front-face constant matches and a test proves it. Flipping twice is invisible until culling is on.
- **`math` knows no graphics API** and builds no projection matrices.
- Angles are radians. Units are metres and seconds.

## Build

Every folder's `CMakeLists.txt` is these four lines and nothing else:

```cmake
cmake_minimum_required(VERSION 3.28)
project(voe_scene C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(scene DEPENDS ecs math base)
```

`voe_module()` (a program folder: `voe_executable()`) does the rest: compiler guards, the one flag set, dependencies, the target `voe_<folder>` and its alias `voe::<folder>` (link the alias), globbing `src/` and `tests/`. **Adding a source or test file needs no CMake edit.** The `DEPENDS` line is the folder's row above, and it is checked.

Daily work: `cmake --preset debug` at the root, then build `voe_<folder>`. Standalone configuration is a property `check.cmake` proves, not a workflow.

## Exempt from the guidelines

- `render/vulkan/` — vendored Khronos headers, unmodified; a fix goes upstream
- `build/`, `cmake-build-*/` — generated

## Never touch

- `history/` — finished cards and bug reports, cited from code; read-only
- `Agentic/decisions/0001`–`0154` — written in the earlier format; never edited, a change is a new record
