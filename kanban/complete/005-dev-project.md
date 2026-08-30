# 005 — the dev project

claimed-by: claude-code (kanban-coder)
status: review

**Needs card 004 (`platform`) finished.** It moves a file that card creates.

## Goal

One program a human runs to see what the engine can currently do. Today that is
a window. Later it is a cleared frame, then a triangle. There is exactly one of
these, and it always shows the current state — not a menu of past states.

Done means: `cmake -P check.cmake` exits zero on both platforms, and running
`voe_dev` opens a window, prints its size when it is resized, and exits zero
when it is closed.

## What this is not

**Not tests.** Tests stay exactly as they are: one file per module, per folder,
fragmented on purpose so an agent working one card runs `ctest -R <folder>` and
reads a short answer. Nothing in this card changes them.

The dev project is the opposite artefact for the opposite audience: one thing, a
person runs it, and it is looked at rather than asserted on.

## Consolidation

Card 004 creates `platform/dev/window.c`. That was the earlier design and it has
been replaced.

- **Move it to `dev/src/main.c`.**
- **Delete `platform/dev/`.**
- If `voe_module()` grew globbing for `<folder>/dev/*.c`, **remove that too.**
  No folder has its own dev programs any more.

Deleting is the point of this card as much as adding is. Two mechanisms for the
same thing is the outcome to avoid.

## `voe_executable()`

`dev/` produces an executable, and there is no way to declare one yet.

Add `voe_executable(<folder> DEPENDS ...)` to `cmake/voe.cmake`, beside
`voe_module()`:

- Same compiler guards, same dependency-map check, same flag set, same
  `src/*.c` glob with `CONFIGURE_DEPENDS`.
- Produces the executable `voe_<folder>` rather than a static library.
- No alias target — nothing links an executable.

`dev/CMakeLists.txt` is then four lines, like every other folder:

```cmake
cmake_minimum_required(VERSION 3.28)
project(voe_dev C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_executable(dev DEPENDS platform base)
```

**The dependency map needs a `dev` row**, allowed to depend on anything, the
same as `app` has. Nothing may depend on `dev` — it is a leaf, and adding it to
another folder's allowed list would be wrong.

## The program

`dev/src/main.c`. Open a window, loop on poll until it should close, print the
size when it changes, exit zero.

- **No engine logic lives here.** It is a call site. If something in it looks
  worth keeping, it belongs in a folder, with a test.
- **No operating-system `#ifdef`.** If one is needed, `platform`'s API has a
  hole and that is the finding to report on this card — do not paper over it
  here.
- It is expected to be rewritten as the engine grows. Old scaffolding gets
  deleted rather than kept behind a flag.

## Check script

`voe_dev` is **built** by the ordinary build, under the same `-Werror` as
everything else — a dev project that does not compile is worse than none,
because it reads as working documentation.

It is **never run** by `check.cmake`. It opens a window and waits for a person.

## Tests

None. Same reasoning as card 004: what this program does is be looked at.

## Report back in this card

- Which C23 features you used.
- Whether `voe_executable()` and `voe_module()` ended up sharing enough to be
  one function with a flag, or are genuinely two. Either answer is fine; say
  which and why, because `app` will use whichever you build.

---

## Result

`status: review`. Markers: **none**. No `DEVIATION:`, no `BLOCKED:`.

### What landed

Added:

    dev/CMakeLists.txt      the four lines, voe_executable(dev DEPENDS platform base)
    dev/dev.md
    dev/src/main.c          moved from platform/dev/window.c

Deleted, which this card counts as much as the adding:

    platform/dev/           the folder is gone, not emptied
    the <folder>/dev/*.c globbing in voe_module()

Changed: `cmake/voe.cmake` (`voe_folder_sources()`, `voe_executable()`, the `dev`
row in the map), `CMakeLists.txt` (`add_subdirectory(dev)`), `check.cmake`
(step 5 reads `DEPENDS` out of either entry point), `platform/platform.md` (the
`dev/window.c` line no longer earns its place).

`dev/src/main.c` contains no operating-system `#ifdef`, so `platform`'s API has
no hole to report from this side.

### Which C23 features were used

**None new.** `main.c` uses nothing that needs C23 on its own; the only C23 in
reach is `bool` as a keyword, and that arrives through `platform/window.h`
rather than being introduced here. The floor stays at clang 18.

### `voe_executable()` and `voe_module()`: genuinely two, over one body

The card asked which, and to say why, because `app` gets whichever exists.

They share more than I expected: the guards, the dependency-map check, pulling
each dependency in as a subdirectory, the `src/*.c` glob, and the
one-platform-only source filter. All of that is now `voe_folder_sources()`, and
both entry points are a call to it followed by about eight lines.

What differs is small, but it is not a flag's worth. A library has an alias, a
public include directory and tests. A program has none of the three, and not
because nobody got round to it — nothing links a program, nothing includes one,
and a folder that is a program has nothing to unit test that would not be better
off in a folder that is a library.

So: two functions, one shared body. The reason is the call site rather than the
implementation. `voe_module(scene DEPENDS ecs math base)` and
`voe_executable(dev DEPENDS platform base)` each say what the folder *is* in the
one line a reader looks at. A flag —
`voe_module(dev EXECUTABLE DEPENDS platform base)` — would put a word in every
folder's four lines that almost none of them need, and the four-line rule earns
its keep precisely because that line is the folder's whole story. **`app` should
use `voe_executable`.**

One thing to know if a third entry point is ever wanted: `voe_folder_sources()`
takes the folder's directory as a parameter rather than reading
`CMAKE_CURRENT_LIST_DIR`. That variable follows the listfile being processed, and
the shared function is two calls deep from the folder's own `CMakeLists.txt`, so
only the caller still knows where the folder is.

### Verified

`cmake -P check.cmake`, all twelve steps:

    ok    tools (clang 22, cmake 4.3.0, slangc, wayland-scanner)
    ok    standalone base
    ok    standalone dev
    ok    standalone math
    ok    standalone platform
    ok    root configure and build
    ok    guard compiler
    ok    guard version
    ok    guard map
    ok    includes
    ok    tests (1 passed)
    ok    harness reports a failure

`standalone dev` appeared with no edit to `check.cmake` — folder discovery found
it because it holds a `CMakeLists.txt`, which is the property that convention
exists for. Same standing caveat as cards 003 and 004: **`slangc` is still not
installed**, so the run above used a stub on `PATH` from the scratch directory,
with nothing in the repository changed.

**Human ran `voe_dev`:** it printed `opened 960x540` and exited cleanly when
closed. See "not verified" below for the one line of the criterion that was not
re-observed.

Each new rule was made to fire rather than assumed:

- **`voe_executable()` enforces the dependency map.** A scratch folder declaring
  `voe_executable(badexe DEPENDS math)` fails with
  `voe_executable(badexe): math is not an allowed dependency (ADR-0022)` — and it
  names itself, because the shared body is told which entry point called it
  rather than always saying `voe_module`.
- **Step 5 now reads `DEPENDS` out of `voe_executable`, and that change is
  load-bearing in both directions.** A `dev/src/probe.h` containing
  `#include <math/float3.h>` fails with
  `dev/src/probe.h: includes math/, which dev does not DEPENDS on`. Reverting
  the regex to `voe_module` only makes `dev`'s declared dependencies read as
  empty, and the *legitimate* `#include <platform/window.h>` is then falsely
  flagged — so the check was proved to both catch what it should and stop
  catching what it should not. Both changes reverted afterwards.
- **`voe_dev` is built and is not a test.** The executable is at
  `build/check/root/dev/voe_dev`; `ctest -N` on the same build lists
  `Total Tests: 1`, `base/arena` and nothing else.
- **The old mechanism is gone, not shadowed.** No `dev/*.c` glob remains in
  `voe.cmake`, `platform/dev/` does not exist, and a search of the root build
  for any `*_dev_*` target comes back empty. Two mechanisms for the same thing
  was the outcome to avoid and there is now one.

Worth recording, because it is the same shape as a finding on card 002: putting
the undeclared include in `main.c` rather than a header fails at **step 3**, not
step 5 — the compiler cannot find `math/float3.h` on a target that does not link
`math`, so the build stops first. Step 5 still earns its place: it names the file
and the rule instead of emitting a missing-header error, and it catches the case
in a header nothing compiles.

### Not verified

- **The `resized` line was not re-observed after the move.** Human's run this
  time was open then close. That exact code was confirmed working on card 004
  (`Meta+Up` printed `resized`, `Alt+F4` printed `closed`, exit 0) and this card
  moved the file between folders without editing the loop, so the risk is small
  — but it is not zero and it is not mine to wave through. One `Meta+Up` on the
  next run settles it.
- **Windows, entirely.** `window_win32.c` still has never been compiled by
  anyone, and `voe_executable()` has never run on Windows.
- **`git` is still unusable in this checkout** — `.git` is a gitlink to a
  `../.git/modules/voe3d` that does not exist. Unchanged from cards 001–004; the
  file move was `mv`, not `git mv`, and nothing was committed.

## Notes — suggestions, not done here

- **The `dev` row in the map includes `app`.** The card said "allowed to depend
  on anything, the same as `app` has", and those two phrases differ by exactly
  one entry, since `app`'s own row cannot contain itself. I took "anything"
  literally. It cannot create a cycle: nothing may depend on `dev` and nothing
  does. Say the word if you meant the narrower reading.
- **Nothing tells a newcomer how to run the dev project.** `CLAUDE.md`'s Build
  section says "then build `voe_<folder>`", which is right for a library and
  says nothing about the one program a person is meant to run. A line there —
  `cmake --preset debug`, `cmake --build build/debug --target voe_dev`,
  `./build/debug/dev/voe_dev` — would answer it in the place people already
  look. Not done here because `CLAUDE.md` is not this card's to edit, and
  `dev.md` is the wrong home for it: a `<folder>.md` is a table of contents and
  nothing else.
- **`voe_dev` still spins a core while open.** Unchanged from card 004 and
  unchanged by this one: `_poll` returns immediately and `platform` has no way to
  wait. The first card that adds time, or a blocking poll, ends it.
