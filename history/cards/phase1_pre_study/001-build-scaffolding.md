# 001 — Build scaffolding

claimed-by: claude-code (kanban-coder)
status: review

## Goal

Create the build system and two placeholder folders. No engine logic.
Done means `cmake -P check.cmake` exits zero on Linux.

Everything needed is in this file, `CLAUDE.md`, `guidelines.md` and
`coding_convention.md`. If it is not, stop and ask.

## Files to create

Exactly these. Nothing else.

```
CMakeLists.txt
CMakePresets.json
check.cmake
.gitignore
cmake/voe.cmake
base/CMakeLists.txt
base/base.md
base/include/base/version.h
base/src/version.c
math/CMakeLists.txt
math/math.md
math/include/math/version.h
math/src/version.c
```

This card names several folders on purpose; that overrides the one-folder rule
in `guidelines.md`.

## `cmake/voe.cmake`

Defines one function, `voe_module(<folder> [DEPENDS <folder>...])`, plus the
data and guards it uses. Wrap the whole file in an include guard so including
it many times is harmless.

**Guards.** Run once, before anything else. Each is `message(FATAL_ERROR ...)`
with the exact text below — `check.cmake` searches for these strings.

| If | Message |
|---|---|
| `CMAKE_C_COMPILER_ID` is not `Clang` | `VOE3D requires Clang (ADR-0005). Found: ${CMAKE_C_COMPILER_ID}` |
| version below 18 (see hook) | `VOE3D requires Clang 18 or newer (ADR-0005). Found: ${version}` |
| `CMAKE_C_COMPILER_FRONTEND_VARIANT` is not `GNU` | `VOE3D requires the GNU-driver clang, not clang-cl (ADR-0026). Found frontend: ${CMAKE_C_COMPILER_FRONTEND_VARIANT}` |

The `ADR-` numbers are just text in the message. Nothing needs to be read.

**Hook.** If the cache variable `VOE_CHECK_FAKE_CLANG_VERSION` is defined, the
version guard uses its value instead of `CMAKE_C_COMPILER_VERSION`. It exists
only so `check.cmake` can test the guard. Comment it as such.

**Map.** The allowed dependencies, as data in the file. Exactly this:

```
base      (none)
math      (none)
ecs       base
platform  base
scene     ecs math base
assets    platform math base
render    platform math base
3d        render scene ecs assets math base
app       base math ecs scene platform assets render 3d
```

**`voe_module(<folder> DEPENDS ...)`**, in order:

1. Run the guards if not already run.
2. Every `DEPENDS` entry must be in `<folder>`'s row of the map. Otherwise
   `message(FATAL_ERROR "voe_module(<folder>): <dep> is not an allowed dependency (ADR-0022)")`.
3. For each dependency, if target `voe_<dep>` does not exist:
   `add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../<dep> ${CMAKE_BINARY_DIR}/<dep>)`.
4. `file(GLOB ... CONFIGURE_DEPENDS src/*.c)` into `add_library(voe_<folder> STATIC ...)`.
   `add_library(voe::<folder> ALIAS voe_<folder>)`.
   `target_include_directories(voe_<folder> PUBLIC include)`.
   `target_link_libraries(voe_<folder> PUBLIC voe::<dep> ...)`.
5. Set on the target: `C_STANDARD 23`, `C_STANDARD_REQUIRED ON`,
   `C_EXTENSIONS OFF`; `target_compile_options(... PRIVATE -Wall -Wextra -Wpedantic -Werror)`.
   No other flags. No platform conditionals.

## Folder files

`base/CMakeLists.txt` — exactly:

```cmake
cmake_minimum_required(VERSION 3.28)
project(voe_base C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(base)
```

`math/CMakeLists.txt` — the same with `math`.

`base/include/base/version.h`:

```c
// Placeholder proving the folder builds. Delete when the first real file lands.
#pragma once
int voe_base_version(void);
```

`base/src/version.c`:

```c
// Placeholder proving the folder builds and that C23 is in force. Delete when
// the first real file lands.
#include <base/version.h>
#include <assert.h>
static_assert(__STDC_VERSION__ >= 202311L, "C23 required");
int voe_base_version(void) { return 1; }
```

`math/` — the same two files with `math`.

`base/base.md` and `math/math.md` — the table-of-contents shape from
`guidelines.md`: a title, one or two sentences, one line per file.

## Root files

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(voe C)
include(cmake/voe.cmake)
add_subdirectory(base)
add_subdirectory(math)
```

`CMakePresets.json` — version 6. Configure presets `debug` and `release`:
generator `Ninja`, `CMAKE_C_COMPILER` = `clang`, `binaryDir` =
`${sourceDir}/build/${presetName}`, `CMAKE_BUILD_TYPE` = `Debug` / `Release`.
Build presets `debug` and `release` pointing at them.

`.gitignore`:

```
build/
```

## `check.cmake`

Run as `cmake -P check.cmake` from the repository root. Uses only CMake
(`execute_process`, `file`, `string`). Deletes and recreates `build/check/`
at start. Prints one line per step — `ok    <step>` or `FAIL  <step>` — stops
at the first failure, and exits non-zero on failure.

All configures below use `-G Ninja -DCMAKE_C_COMPILER=clang` unless the step
says otherwise.

| Step | Do | Pass when |
|---|---|---|
| 1 tools | Run `clang --version`, `cmake --version`, `slangc -v` | all three run; clang major ≥ 18; cmake ≥ 3.28 |
| 2 standalone | For each of `base`, `math`: configure that folder alone into `build/check/<folder>` | exit code 0 |
| 3 root | Configure root into `build/check/root` with `-DCMAKE_BUILD_TYPE=Debug`, then `cmake --build build/check/root` | both exit 0 |
| 4a guard compiler | Configure root with `-DCMAKE_C_COMPILER=gcc`. If `gcc` is not on PATH print `skip  guard compiler (no gcc)` and continue | exit ≠ 0 and output contains `ADR-0005` |
| 4b guard version | Configure root with `-DVOE_CHECK_FAKE_CLANG_VERSION=17` | exit ≠ 0 and output contains `Clang 18 or newer` |
| 4c guard map | Write `build/check/badedge/CMakeLists.txt` = the four lines with `project(voe_badedge C)`, `include(<absolute path to cmake/voe.cmake>)`, `voe_module(badedge DEPENDS math)`; create empty `build/check/badedge/src/`; configure it | exit ≠ 0 and output contains `ADR-0022` |
| 5 includes | For every `.c`/`.h` under `base/` and `math/`: (a) fail if it includes `<windows.h>`, `<unistd.h>`, `<dlfcn.h>`, `<pthread.h>`, or anything starting `<X11/`, `<xcb/`, `<wayland`. (b) For every `#include <X/...>` or `#include "X/..."` where `X` is a folder name from the map, `X` must be the file's own folder or appear in that folder's `voe_module(... DEPENDS ...)` line | no violations |
| 6 tests | Print `ok    tests (none yet)` | always |

Where the folder list appears in steps 2 and 5, discover it as "every
top-level directory containing a `CMakeLists.txt`" rather than hard-coding
`base` and `math`, so later folders are picked up automatically.

## Done when

- From a clean checkout: `cmake --preset debug && cmake --build --preset debug` succeeds.
- `cmake -P check.cmake` prints `ok` (or `skip` for 4a) on every step and exits zero.
- Only the files listed above exist or changed.
- The final answer lists every `DEVIATION:` marker left in the code, or says there are none.

## Not in this card

Tests, sanitizers, executables, Windows, any other folder.

## Notes

### State

All 13 files from the card's list are written except `.gitignore`, which was
left alone deliberately — see the open question below. `cmake -P check.cmake`
exits zero on Human's machine, so the Goal's gate holds.

### The gate, on Human's machine

    ok    tools (clang 22, cmake 4.3.3, slangc)
    ok    standalone base
    ok    standalone math
    ok    root configure and build
    ok    guard compiler
    ok    guard version
    ok    guard map
    ok    includes
    ok    tests (none yet)

`guard compiler` printed `ok` rather than `skip`, so gcc was on PATH and the
compiler guard genuinely fired.

The coder's machine could not run this: no `slangc`, no `ninja`, and a WSL2
drive mount with no `chmod`, which stops CMake's `configure_file` from writing
anywhere inside the repository. That is recorded only so the next reader knows
why the verification below is shaped the way it is; it is not a property of the
scaffolding.

### Verified on the coder's machine, by substitution

A temporary copy of `check.cmake`, patched in exactly three places — Unix
Makefiles for the absent Ninja, the `slangc` probe replaced by `cmake -E true`,
and `checkdir` moved outside the repository — printed `ok` on all nine lines and
exited zero. The copy was deleted; the real `check.cmake` was never modified.

Each guard was then run alone, to confirm it fires for its stated reason rather
than merely failing configuration:

- gcc -> `VOE3D requires Clang (ADR-0005). Found: GNU`
- `-DVOE_CHECK_FAKE_CLANG_VERSION=17` -> `VOE3D requires Clang 18 or newer
  (ADR-0005). Found: 17`; at `=18` configuration succeeds, so the boundary is
  not off by one.
- badedge -> `voe_module(badedge): math is not an allowed dependency (ADR-0022)`

Step 5 was negative-tested, because a check that only ever passes proves
nothing. It caught, and reported by path, each of: `<unistd.h>` in `math`;
`<X11/Xlib.h>`; `#include <base/version.h>` from `math`; and the quoted form
`#include "base/version.h"`. Reverted after each.

Flags reaching the compiler were read out of `compile_commands.json`:
`-std=c23 -Wall -Wextra -Wpedantic -Werror` and nothing else, with only the
folder's own `include/` on the include path. Both `libvoe_base.a` and
`libvoe_math.a` link.

### Thinner than the rest

- **The presets were not exercised end to end.** `CMakePresets.json` parses as
  version 6 and `cmake --list-presets` resolves both `debug` and `release`, and
  check.cmake's step 3 configures and builds the root with the same generator,
  compiler and build type the `debug` preset sets — but
  `cmake --build --preset debug` itself has not been run. One command closes it.
- Windows was not touched at all.

### Open question: `.gitignore`

The card specifies `.gitignore` containing `build/`. A richer one already exists
and already ignores `/build/`, along with `/out/`, `/cmake-build-*/`,
`compile_commands.json`, and a deliberate "ignore everything dotted, then
re-include git's own metadata" policy. Overwriting it with the one-line version
would delete all of that, so it was left alone: the card's intent — build output
is ignored — already holds. This is the one place the diff departs from the
card's letter, and it is the reviewer's to settle.

### Suggestion, not done here

Step 5a forbids `<unistd.h>` and friends in **every** discovered folder, which is
what the card asks for. `platform` is meant to be the one OS-aware folder, so it
will trip this rule the day it lands. Whoever writes the `platform` card decides
on the exemption; it is not this card's business.
