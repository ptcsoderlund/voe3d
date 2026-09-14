# 0005. Clang is the only supported compiler, on both platforms, enforced by CMake

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

C23 is a given, not a decision. MSVC has no `/std:c23` — only `/std:clatest`,
a partial and moving subset. So the question is not *whether* C23, but which
compilers can deliver it, and at what cost.

Constraints fixed before this decision:

- **C23, Vulkan, Windows + Linux desktop, CMake** (pre-study givens).
- **No CI (ADR-0004).** A supported compiler is one the principal personally
  runs before a card moves to `complete/`. Every entry in the supported set is
  recurring manual work. Two families is a real cost; three is a fiction.
- **Onboarding invariant.** Checked, and it **does not discriminate here** —
  every option below is a single installer on Windows. The invariant decided
  ADR-0001 through ADR-0003; it does not decide this one, and reaching for it
  reflexively would have been wrong.

## Options considered

### Option A — MSVC on Windows, GCC on Linux
The conventional split. Windows contributors use the toolchain they already
have; VS debugger, default generator, the shape every third-party CMake project
expects.

The language actually written becomes **C23 ∩ MSVC**, and no flag expresses
that intersection. It is a convention enforced by nothing except somebody
remembering to build on Windows.

### Option B — Clang on both platforms
`clang` on Linux, `clang-cl` or clang targeting the MSVC ABI on Windows. One
family, one dialect, `-std=c23` meaning the same thing in both places. Identical
diagnostics, identical UB, sanitizers on both sides, one warning-flag set.
Visual Studio ships clang as the *C++ Clang tools for Windows* component, so
Windows remains one installer with a checkbox, and MSVC ABI keeps the VS
debugger working.

### Option C — GCC on both platforms, MinGW-w64 on Windows
Single family, full C23, the most self-contained Windows install of the three
— no Windows SDK at all. Vulkan is fine; the loader is a runtime DLL.

## Decision

**Option B.** Clang is the only supported compiler on both platforms. MSVC is
not supported. GCC is not supported, including on Linux.

The deciding factor: **Option A makes the language subset invisible and
unenforceable.** With C23 given and no CI, A means writing to an intersection
that no flag expresses, no tool checks, and no diff reveals — a second
unenforced invariant sitting beside the one ADR-0004 already named as this
project's known weak point. B turns the dialect from a convention into a
compiler flag, which is the only form of rule this project has agreed to trust.

**CMake enforces it.** Configuration fails — it does not warn, and does not fall
back — when the compiler is not Clang, or is older than the floor:

```cmake
if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "VOE3D requires Clang (ADR-0005). Found: ${CMAKE_C_COMPILER_ID}")
endif()
if(CMAKE_C_COMPILER_VERSION VERSION_LESS 18)
    message(FATAL_ERROR "VOE3D requires Clang 18 or newer (ADR-0005). Found: ${CMAKE_C_COMPILER_VERSION}")
endif()
```

`CMAKE_C_COMPILER_ID` is `Clang` for both the MSVC-driver (`clang-cl`) and
GNU-driver forms, so one guard covers both platforms. `AppleClang` is a distinct
id and is out of scope by the platform given.

**Verified on Linux** (CMake 4.2.3): the guard configures cleanly under
`-DCMAKE_C_COMPILER=clang` (Clang 21.1.8, `CMAKE_C_COMPILER_FRONTEND_VARIANT`
= `GNU`) and aborts under `-DCMAKE_C_COMPILER=gcc` with
`VOE3D requires Clang (ADR-0005). Found: GNU`. A probe translation unit
exercising `constexpr` objects, `auto`, `typeof`, `nullptr`, fixed-underlying-type
enums, binary literals, one-argument `static_assert`, `[[nodiscard]]`,
`<stdbit.h>` and `<stdckdint.h>` compiles clean with `C_STANDARD 23`,
`C_STANDARD_REQUIRED ON`, `C_EXTENSIONS OFF`. The Windows half is unverified —
see D-003a.

**Minimum version: Clang 18** — the first release accepting `-std=c23` as a
spelling rather than `-std=c2x`. This floor is provisional against the Windows
side only: it is confirmed on Linux (Clang 21.1 present) and needs confirming
against whatever the Visual Studio clang component actually installs. If that
turns out to be older than 18, the fallback is the standalone LLVM installer,
which is still one install and leaves the invariant intact. Confirming this is a
task, not a reopening of the decision.

The standard is requested through CMake's standard properties rather than raw
flags; the surrounding layout, presets and where the guard physically sits are
D-005.

## Blast radius

**Reversibility: load-bearing in one direction, free in the other.**

Dropping Clang for something else costs nothing today, while no code exists.
Adding **MSVC** support later is the expensive direction: it is a sweep through
every translation unit removing C23-only constructs, and the cost grows with the
codebase. This asymmetry is the real price of the decision and was weighed
against A explicitly. It is accepted, and is not to be relitigated — a genuine
future requirement for MSVC-native builds is grounds for a superseding ADR, not
for reopening this one.

## Consequences

- **MSVC is not supported and cannot be added cheaply later.** Stated plainly so
  nobody discovers it by surprise.
- **GCC is unsupported even on Linux**, where it is the default `cc`. A
  contributor's habitual `cmake ..` on a stock Linux box will fail the guard.
  That failure is the feature — it is loud, immediate, and names this ADR.
- **One compiler family is not one command line.** Windows clang uses the MSVC
  driver, Linux clang the GNU driver, and `CMAKE_C_COMPILER_FRONTEND_VARIANT`
  differs between them. Warning flags and flag handling must account for it. The
  dialect is unified; the driver is not.
- Third-party dependencies fetched at configure time that assume MSVC are
  largely satisfied by clang-cl's cl-compatible driver. This is a concrete
  advantage over Option C that has already paid for itself.
- Sanitizers (ASan, UBSan) are available on both platforms with identical
  syntax. Given the assert-heavy style required by `coding_convention.md`, this
  is a direct gain, not a nicety.
- **Windows is still only verified when the principal builds there.** ADR-0004
  stands; a single compiler family reduces the matrix but does not automate it.
- **The MSVC C23 feature-probe spike is dissolved.** It existed to price Option
  A's intersection. With MSVC unsupported, full C23 is available and the
  question it answered no longer has a consumer. A narrower probe may still be
  wanted to pin the Windows Clang version floor.
- The rule must reach every contributor holding only the engine clone, so it
  belongs in the engine's `CLAUDE.md` per ADR-0003. That is a card for the
  principal to write; this ADR does not place it.

## Rejected options and why

- **Option A (MSVC + GCC)** — rejected because it makes the usable language an
  unwritten intersection of two toolchains, enforceable by nothing. It is the
  cheaply-reversible option and that was argued honestly in its favour; it lost
  because an invisible dialect floor is exactly the class of unenforced
  invariant ADR-0004 already committed this project to avoiding.
- **Option C (GCC + MinGW-w64)** — rejected because it reaches the same single
  dialect as B while giving up Windows platform citizenship: no VS debugger, a
  toolchain the next Windows programmer does not have, and friction with
  dependencies that assume the MSVC ABI. It spends something B does not have to.
- **Supported/best-effort tiers across three or four compilers** — rejected on
  sight. With no CI, "best effort" means "broken", documented as a promise.

## Questions this opens

- **D-020** — which Clang driver on Windows: `clang-cl` (MSVC driver) or
  `clang` (GNU driver) targeting the MSVC ABI? Affects flag handling in D-005.
- **D-003a** — confirm the Clang version shipped by the Visual Studio clang
  component meets the 18 floor. A task on the principal's Windows machine, not
  a decision.
- Unblocks **D-005** (CMake layout) and, through it, **D-013** (check script,
  which must verify the guard actually fires).
