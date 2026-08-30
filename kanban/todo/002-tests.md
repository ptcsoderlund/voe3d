# 002 — the test mechanism

claimed-by:
status: todo

## Goal

Make it possible to write a test, and make `check.cmake` fail when one fails.
None of this exists yet: step 6 of the check prints `tests (none yet)` and is a
no-op, so the script currently exits zero on a folder whose code is wrong.

This card builds the mechanism and proves it. It writes **no tests for engine
code** — the next card does that, against real functions.

Done means: `cmake -P check.cmake` exits zero, its tests step is no longer a
no-op, and a deliberately failing test makes the whole script exit non-zero.

## What a test is

An ordinary C program.

```c
#include <testing/test.h>

int main(void)
{
    VOE_TEST_CHECK(2 + 2 == 4);
    return voe_test_result();
}
```

- One file per module, at `<folder>/tests/<module>.c`.
- Each file is its own executable with its own `main`. A crash therefore fails
  one test, not a folder's worth.
- Zero means pass, non-zero means fail.
- **Nothing is registered anywhere.** There is no list of tests to add to. The
  build finds the files. Writing a test creates one file and changes nothing
  else in the repository — that property is the point, do not design it away.

## `testing/` — the check macros

A single header, `testing/include/testing/test.h`. Header-only: no `src/`, no
`.c` file.

**The failure message is the whole deliverable.** Whoever reads it — a person or
an agent — reads it and nothing else, and must be able to fix the code without
opening the test. So a failed check prints the expression, the file, the line,
and for a comparison **both values**. `check failed at vec3.c:12` is not good
enough and is the main way this card can be done badly.

Provide what the next card needs and no more: a boolean check, an integer
comparison, and a float comparison with a tolerance. Add more when something
needs more.

`voe_test_result()` returns 0 if every check passed, non-zero otherwise. A test
runs all its checks rather than stopping at the first — one run should report
everything that is broken.

## Wiring

- `voe_module()` picks up `<folder>/tests/*.c` when that folder exists, builds
  one executable per file, and registers each with `add_test()`. Test names are
  `<folder>/<file>`, so `ctest -R math` runs one folder's tests.
- **No fifth line.** A folder's `CMakeLists.txt` stays four lines and gains no
  `TESTS` argument.
- Test executables link `voe::testing`. Library targets must not — `src/` never
  includes `testing/`, and step 5 of the check should enforce that the same way
  it enforces everything else.
- A folder with no `tests/` is silent, not an error.

**One trap.** `check.cmake` discovers folders as "every top-level directory
holding a `CMakeLists.txt`", and `voe_module()` builds a STATIC library from
globbed `src/*.c`. A header-only `testing/` fits neither: it is not a folder in
the dependency map, and it has no sources. Solve it however is cleanest —
keeping `testing/` out of folder discovery entirely is one way — but do not
solve it by weakening the map check or by giving `testing/` a source file it
does not need.

## `check.cmake`

- **Step 6 runs `ctest`** against the root `debug` build, with failure output
  shown, and fails the script when any test fails.
- **Add a negative test, in the style of steps 4a–4c.** Create a scratch folder
  containing a test that deliberately fails, configure it, build it, run
  `ctest`, and assert that it reported failure. A harness that always reports
  green is the one failure mode that makes every future card worthless, so it is
  proven here rather than assumed.

## Constraints

- No new required tool. `ctest` ships with CMake, which is already required.
- No test framework, first-party or vendored.
- Windows and Linux, identically.

## Report back in this card

Which C23 features you used, and the exact text of a failing check as it appears
in the terminal. The second one is the deliverable being reviewed.
