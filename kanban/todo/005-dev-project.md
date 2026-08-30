# 005 — the dev project

claimed-by:
status: todo

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
