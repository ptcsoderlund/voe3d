# 0001. Engine modules live in one repository as self-contained CMake subprojects

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

VOE3D is to be built as a modular engine. The question is whether those modules
are separate git repositories joined by submodules, or directories within a
single engine repository.

Constraints fixed before this decision:

- C23, Vulkan, Windows + Linux desktop, CMake with configure-time dependency
  acquisition.
- **Onboarding invariant:** a programmer must be able to clone and build with
  nothing installed beyond a compiler and CMake.
- VOE3D currently has exactly one consumer: VOE3D.

The decisive framing is that modularity is a property of the **build graph**,
not of the **repository graph**. Only the build graph enforces a boundary at
compile time; repository walls enforce nothing that CMake target visibility does
not already enforce.

## Options considered

### Option A — Single repo, modules as plain CMake targets
Each module an `add_library()` under `src/`, dependencies expressed in
`target_link_libraries`. One clone, one history, one CI.

### Option B — One repo per module, joined by git submodules
A super-repo pins each module by commit SHA.

### Option C — Single repo, each module a self-contained CMake subproject
Option A plus a discipline: every module carries its own `CMakeLists.txt` that
also configures standalone, public headers live under `include/<module>/`, and
no module reaches into a sibling's source directory.

## Decision

**Option C.** The deciding factor is that submodules purchase independent
versioning, and VOE3D has no second consumer to sell that to — while charging
for it in exactly the currency the project values most, the clone-and-go
experience.

Option C yields the same boundary guarantees as B, enforced by the linker rather
than by repository walls, and leaves B available later as a directory move
rather than as an upfront tax.

## Blast radius

**Reversibility: cheap-to-moderate.** Extracting a module later is
`git filter-repo` on one directory, then pointing `FetchContent` at the new
repository. Because each module already configures standalone, no source changes
are required to make that move. Going the other direction — collapsing
submodules back into a monorepo — is the expensive direction, and this decision
avoids ever needing it.

## Consequences

- Cross-module refactors are a single atomic commit, always in a consistent state.
- CI is one pipeline over one checkout.
- The standalone-configure property of each module is not self-enforcing: it
  rots silently unless a CI job actually configures modules in isolation. That
  job is a requirement of this decision, not an optional extra.
- Per-module CMake ceremony is higher than Option A. Accepted as the price of
  keeping extraction cheap.
- Nothing prevents a contributor from adding an upward or sideways dependency
  except the module DAG itself. Per the standing rules, any such proposal is an
  ADR, not an implementation detail.

## Rejected options and why

- **Option A** — rejected not on its merits but as insufficient. It reaches the
  same clone-and-go outcome while quietly losing the extraction escape hatch,
  since nothing keeps modules from growing tangled include paths into each
  other. C is A with the discipline written down and CI-checked.
- **Option B** — rejected because it breaks the onboarding invariant outright:
  `git pull` does not update submodules, so contributors routinely build stale
  or empty module trees, and detached-HEAD confusion becomes the dominant
  support burden. Every cross-module change becomes N commits plus a pointer
  bump. It buys independent versioning and per-repo access control, neither of
  which has a consumer today. This is premature generality.

## Questions this opens

- **D-002** — the module map and its dependency DAG.
- **D-005** — CMake layout: how "configures standalone" is expressed and tested.
- **D-013** — the CI job that proves standalone configuration still works.
