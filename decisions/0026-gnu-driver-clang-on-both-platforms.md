# 0026. The GNU-driver `clang` is used on both platforms; `clang-cl` is not supported

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

ADR-0005 fixed Clang as the only compiler and left one thing open (D-020): on
Windows, Clang ships in two forms that produce the same code for the same MSVC
ABI but speak different command lines.

- `clang-cl` — the MSVC-compatible driver: `/W4`, `/Zi`, `/std:`.
  `CMAKE_C_COMPILER_FRONTEND_VARIANT` is `MSVC`.
- `clang` — the GNU-compatible driver targeting `x86_64-pc-windows-msvc`:
  `-Wall`, `-g`, `-std=c23`, identical to Linux.
  `CMAKE_C_COMPILER_FRONTEND_VARIANT` is `GNU`.

What depends on it: every compile-flag line in D-005's CMake layout, and whether
D-013's check script branches on platform.

Constraints already fixed:

- **ADR-0005** — Clang only, floor 18, guard in CMake. Its stated cost was that
  "one compiler family is not one command line". This decision is the chance to
  not pay that cost.
- **ADR-0004** — no CI. Windows is verified only when the principal builds
  there, so a supported configuration that nobody runs is a broken one.
- **ADR-0023** — write it ourselves. The one concrete advantage ADR-0005 gave
  `clang-cl` was compatibility with fetched third-party CMake logic that assumes
  MSVC. That advantage has lost its consumer: there is no third-party build
  logic left to keep happy, and the Vulkan spike (D-006) showed even Vulkan
  brings no build-time dependency.

Verified against CMake 4.2.3's own modules before deciding, not from memory:

- `Modules/Compiler/Clang-C.cmake`: under the GNU variant, `C_STANDARD 23`
  becomes `-std=c23` for Clang ≥ 18 and **silently degrades to `-std=c2x` for
  Clang 9–17**. Under the MSVC variant it becomes `-clang:-std=c23` for ≥ 18 and
  is **unset** below that, so configuration fails outright.
- `Modules/Platform/Windows-Clang.cmake`: the GNU driver on Windows is fully
  supported — default linker `lld-link`, CodeView debug info (`-g -Xclang
  -gcodeview`), PDB emission, MSVC runtime-library selection, `.lib`/`.exe`
  naming. Binaries are ordinary MSVC-ABI binaries and debug in Visual Studio.

## Options considered

### Option A — `clang-cl` on Windows, `clang` on Linux
The conventional split; Chromium and Firefox build this way. Best-trodden,
fewest surprises, and Visual Studio's generator can produce a `.sln`. Costs: two
spellings of every warning, sanitizer and debug flag; a platform branch in each
of the eight folders' CMake files or in a shared flags file; a check script that
must know both dialects. The dialect is unified; the command line never is.

### Option B — GNU-driver `clang` on both platforms
One vocabulary everywhere. `CMAKE_C_COMPILER_FRONTEND_VARIANT` is `GNU` on both
platforms, so one flag set, one sanitizer spelling, no platform branch in any
folder, and one check script. Windows output is unchanged: MSVC ABI, lld-link,
PDBs. Cost: the less-travelled configuration on Windows, and with no CI any
wrinkle lands on the principal at build time.

### Option C — accept either driver
Rejected on sight. ADR-0005 already ruled that with no CI, two supported
configurations means one is quietly broken.

## Decision

**Option B. The GNU-driver `clang` is the only supported form on both
platforms. `clang-cl` is not supported.** Deciding factor: ADR-0005 bought "one
dialect, enforced by a flag" and accepted two command lines as the price;
ADR-0023 has since removed the only reason that price was worth paying.

**CMake enforces it**, alongside the ADR-0005 guard:

```cmake
if(NOT CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    message(FATAL_ERROR "VOE3D requires the GNU-driver clang, not clang-cl (ADR-0026). Found frontend: ${CMAKE_C_COMPILER_FRONTEND_VARIANT}")
endif()
```

**Binding condition:** compiler and linker flags live in **exactly one file**,
included by every folder's standalone `CMakeLists.txt`. No folder spells a flag
itself. This is what keeps the decision cheap to reverse and is a requirement on
D-005, not a suggestion.

**Install path on Windows: the standalone LLVM release**, not the Clang bundled
with Visual Studio. The LLVM Windows installer ships `clang.exe` defaulting to
`x86_64-pc-windows-msvc`, `lld-link`, and the C23 headers (`<stdbit.h>`,
`<stdckdint.h>`) in its resource directory, and it publishes the **same version
on both platforms** — at the time of writing 22.1.5 stable, 23 in release
candidate. Linux may use the distribution's package or the same LLVM release;
either is fine above the floor.

## Blast radius

**Reversibility: cheap, on the binding condition above.** Switching to
`clang-cl` later is editing one flags file and one guard. Without the condition
it is a sweep across eight folders — which is exactly why the condition is
binding.

## Consequences

- **The ADR-0005 version guard is now load-bearing, not belt-and-braces.** Under
  the GNU driver a pre-18 Clang configures successfully with `-std=c2x`, a
  *draft* of C23. Only the explicit `VERSION_LESS 18` check stops a silently
  half-C23 build. **D-013's check script must prove that guard fires**, not just
  the compiler-id one.
- **The Windows SDK and MSVC CRT are still required.** The GNU driver targets
  the MSVC ABI and links against Microsoft's C runtime and `kernel32`/`user32`;
  those headers and import libraries come from Visual Studio or the Visual
  Studio Build Tools, which `clang.exe` locates automatically. This is the
  Windows equivalent of Linux's `libc6-dev` and sits on the *tools* side of
  ADR-0021's line. It is stated plainly in the required-tools list so it is a
  known install, not a surprise.
- **No Visual Studio `.sln` generation.** The GNU driver is not usable with the
  Visual Studio generator; the build is Ninja on both platforms. Debugging still
  works — open the executable in VS or use `lldb`, which the LLVM release ships.
  A folder-open in VS Code or CLion with the CMake presets is the intended IDE
  path.
- **Warning flags, sanitizer flags, and debug flags are written once.** ASan and
  UBSan spell identically on both platforms — a direct gain for the
  assert-heavy style `coding_convention.md` requires.
- **D-003a changes shape.** It was "is Visual Studio's bundled Clang ≥ 18". It
  becomes "install the LLVM release on Windows and confirm the ADR-0005 probe
  builds under the GNU driver". Visual Studio's bundled Clang version no longer
  matters.
- **The `apt` package on Linux is behind the LLVM release** (21.1 versus 22.1 on
  the principal's machine today). Harmless above the floor; noted so nobody
  reads a version mismatch as a fault.

## Rejected options and why

- **Option A (`clang-cl` on Windows)** — rejected because its unique advantage,
  compatibility with third-party CMake that assumes MSVC, has no consumer under
  ADR-0023, while its cost — two command lines — is paid in every folder,
  forever, and doubles the check script.
- **Option C (accept either)** — rejected under ADR-0005's own rule: two
  supported configurations with no CI is one broken configuration.
- **Visual Studio's bundled Clang as the recommended install** — rejected as the
  *recommended* path (it is not forbidden if it meets the floor). Its version
  is tied to the VS release cadence, it is oriented around `clang-cl`, and the
  standalone LLVM release lets both platforms run the same version, which the
  principal explicitly wants.

## Questions this opens

- **Closes D-020.**
- **D-033 (new)** — the Clang floor. ADR-0005's 18 is "first release spelling
  `-std=c23`", not "first release implementing the C23 features we use".
  Clang's C23 support landed across 18–20 (C `constexpr` and `#embed` are
  later than 18). Given the principal's position that the newest common release
  is always installable, the floor should be re-argued against the features
  actually used, once the first `base`/`math` card exists to name them. Amends
  ADR-0005 if raised; a task, not a decision, if 18 turns out to suffice.
- **D-003a** reworded per the consequence above. Still blocked on the
  principal's Windows machine.
- **D-005** inherits two requirements: the single flags file, and the
  frontend-variant guard beside the ADR-0005 guard.
- **D-013** inherits one: prove the `VERSION_LESS 18` guard fires, not only the
  compiler-id guard.
