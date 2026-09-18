# 0027. CMake layout: one shared function, a four-line file per folder

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Amended by:** ADR-0029 — names are `voe_module()`, `cmake/voe.cmake`, `voe_<folder>`, `voe::<folder>`.
- **Supersedes:** —
- **Superseded by:** —

## Context

D-005. How each of the eight folders (ADR-0022) configures standalone
(ADR-0001) while sharing exactly one flags file (ADR-0026) and reaching its
sibling dependencies. Everything in Phase 2 — the first scaffolding cards and
the check script (D-013) — depends on it.

Constraints already fixed:

- **ADR-0001** — every folder is its own CMake project and configures standalone.
- **ADR-0022** — eight folders, a fixed acyclic dependency map.
- **ADR-0026** — GNU-driver Clang on both platforms, Ninja, all compiler and
  linker flags in exactly one file, and two configure-time guards (compiler id
  and version from ADR-0005, frontend variant from ADR-0026).
- **ADR-0014** — namespace spelled out in names; targets follow: `v_<folder>`.
- **ADR-0004** — no CI. The check script is the only enforcement; the layout
  must make its job small.
- **Principal's criterion, raised in this decision:** minimise what an AI coder
  agent has to read to implement a card. The per-folder CMake file is what an
  agent actually opens; boilerplate repeated eight times is read eight times
  and drifts eight ways.

## Options considered

### Option A — relative `add_subdirectory`, guarded
Each folder is a full `project()`; when top-level it includes the flags file
and pulls each dependency in with `if(NOT TARGET v_base) add_subdirectory(...)`.
The de-facto practice of CMake monorepos (O3DE, Wicked, Ogre-Next). Written
plainly it costs ~6 lines per dependency per folder — `render` has six.

### Option B — siblings via `FetchContent` with local `SOURCE_DIR`
Same shape; deduplication comes free and extraction later is changing
`SOURCE_DIR` to `GIT_REPOSITORY`. Closest to the CMake documentation's
recommended way of consuming dependencies — which is written for third-party
code, the case ADR-0023 has largely removed. Machinery on day one for an
extraction that has not happened.

### Option C — installed packages, `find_package`
Each folder installs an export set; configuring `scene` standalone requires its
dependencies built and installed first. Best practice for libraries shipped to
others; nobody consumes `v_base` but us. Densest per-folder file of the three
(install rules, export, config and version files) and an install step in the
daily loop and the check script.

## Decision

**Option A, with a declarative face.** One function, `voe3d_module()`, in the
single shared file `cmake/voe3d.cmake` at the engine root. Each folder's
`CMakeLists.txt` is four lines:

```cmake
cmake_minimum_required(VERSION 3.28)
project(voe3d_scene C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe3d.cmake)
voe3d_module(scene DEPENDS ecs math base)
```

`voe3d_module(<folder> [DEPENDS <folder>...])` does everything else, once:

1. Runs the ADR-0005 guards (Clang, ≥ 18) and the ADR-0026 guard (frontend
   variant `GNU`). Idempotent, so it runs on a standalone configure too.
2. Applies the one flag set: `C_STANDARD 23`, `C_STANDARD_REQUIRED ON`,
   `C_EXTENSIONS OFF`, warnings, sanitizer and debug options.
3. **Checks every `DEPENDS` edge against ADR-0022's map** and fails
   configuration on an edge the map does not allow, naming the ADR.
4. Pulls in each dependency with a guarded `add_subdirectory(../<dep>)` so the
   folder configures standalone from inside the tree.
5. Creates the static library `v_<folder>` and the alias `voe3d::<folder>`;
   consumers link the alias. Public headers under `include/`, sources globbed
   from `src/` with `CONFIGURE_DEPENDS`.

The root `CMakeLists.txt` lists the eight folders in ADR-0022's build order
and carries the only `CMakePresets.json`: Ninja, Clang, `debug` and `release`.
Standalone configuration is a property the check script proves, not a daily
workflow; daily work is `cmake --preset debug` at the root and building
`v_<folder>`.

**Deciding factor:** it is the least an agent can be asked to read — the
four lines *are* that folder's row in the module map, so opening the build file
also tells the agent what it may include — and it turns ADR-0022 from a rule
people remember into one configuration checks.

**Not premature generality:** the function has eight call sites on day one.

## Blast radius

**Reversibility: cheap.** Every alternative mechanism — FetchContent, or
install/export for a folder that graduates to being consumed elsewhere — is a
change inside `voe3d_module()`. No folder file changes. Consumers already link
`voe3d::<folder>`, which is exactly the name an installed package would present,
so a later move to Option C is invisible to them.

**CMake floor: 3.28.** What Ubuntu 24.04 LTS ships; the principal runs 4.2.
Raising it is one line in eight files; the check script confirms it.

## Consequences

- **The module map is machine-checked.** An upward or sideways `DEPENDS` fails
  at configure time with a message naming ADR-0022. The sixth swap of a
  remembered rule for a checked one on this project. The map lives in
  `cmake/voe3d.cmake` as data; changing it is an ADR, then a one-line edit.
- **Adding a source file needs no CMake edit.** A deliberate departure from the
  "list sources explicitly" school. The objection to globbing — new files
  silently missed — is what `CONFIGURE_DEPENDS` fixes under Ninja, which
  ADR-0026 already fixed as the generator. A coder agent adds `foo.c` and
  builds; fewer edits, fewer things to get wrong.
- **The check script's job shrinks (D-013):** configure each folder standalone
  and confirm it succeeds; configure with a forbidden edge and confirm it
  fails; configure under GCC, under `clang-cl`, and under a `VERSION_LESS 18`
  Clang (or a simulated one) and confirm each guard fires.
- **`cmake/voe3d.cmake` is the densest file in the build and the only one that
  is.** It is the one place a coder agent should not need to open. Its
  behaviour is documented in the engine's `CLAUDE.md` in the four lines above
  plus one sentence: "`DEPENDS` must be in the module map or configure fails."
- **Standalone configure works only inside the tree** — the relative
  `../cmake/` include and `../<dep>` paths assume the monorepo. Extraction of a
  folder (ADR-0001's escape hatch) moves the function's mechanism for that
  folder to FetchContent or an installed package. Priced, accepted.
- **Test targets and executables are not in this ADR.** `voe3d_module()`
  builds a static library. Tests are D-011; `app`'s executable and any tool
  executables are a second function or an option on this one, decided when
  the card needs it. Public-header conventions remain D-012.
- **Target naming is settled:** `v_<folder>` and `voe3d::<folder>`. Test
  target naming waits for D-011.

## Rejected options and why

- **Option A written out plainly** — the same mechanism, but ~40 lines of
  guards in `render` alone, repeated eight ways. Rejected on the principal's
  criterion: repeated boilerplate is read by every agent and drifts.
- **Option B (FetchContent on local dirs)** — rejected as machinery for an
  extraction that has not happened. Its one advantage is preserved: the
  function can switch to it internally without touching any folder.
- **Option C (installed packages)** — rejected as the industry practice for
  the wrong shape. It is correct for libraries with external consumers; with
  one consumer it adds the densest boilerplate of the three and an install
  step to the check script. Kept reachable through the alias naming.
- **Per-folder `CMakePresets.json`** — rejected. Standalone configure is the
  check script's concern; a preset in each folder is eight files to keep in
  step for a workflow nobody uses daily.

## Questions this opens

- **Closes D-005.** Unblocks **D-013**, which inherits the test list above.
- **D-011** (tests) now has a concrete shape to fit into: a second function or
  a `TESTS` argument on `voe3d_module()`, decided against the first `math`
  card.
- **D-034 (new)** — how `app` and any tool executables are declared. Small;
  decided when the `app` scaffolding card is written. Likely
  `voe3d_executable()` beside `voe3d_module()`.
- **D-021** (Clang-only rule into the engine's `CLAUDE.md`) grows to carry the
  four-line file and the `DEPENDS` sentence.
