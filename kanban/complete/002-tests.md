# 002 — the test mechanism

claimed-by: claude-code (kanban-coder)
status: review

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

## Notes

### What was built

    testing/include/testing/test.h   the three checks and voe_test_result()
    testing/testing.md
    cmake/voe.cmake                  enable_testing(), voe::testing, test wiring
    check.cmake                      step 5 rule, real step 6, new step 6b

No tests for engine code, per the card. `math/tests/` and `base/tests/` do not
exist; the mechanism is proven against a scratch folder instead.

### The trap, and how it was solved

`testing/` has no `CMakeLists.txt`. Folder discovery in `check.cmake` is "every
top-level directory holding a `CMakeLists.txt`", so `testing/` is simply not a
folder: nothing tries to configure it standalone, and the dependency map needs
no row for it. `voe::testing` is an `INTERFACE` target created in
`cmake/voe.cmake` itself, pointing at `testing/include`. Nothing was weakened
and `testing/` got no source file it does not need.

`enable_testing()` is called at the top of `cmake/voe.cmake`, outside every
function. Because of `include_guard(GLOBAL)` it runs exactly once, in the first
scope that includes the file — the repository root in a root build, the folder
itself in a standalone one. Both are top level, so `ctest` finds tests either
way and no folder's `CMakeLists.txt` learns that tests exist.

`voe_target_settings()` now holds the flag set and language level, and both the
library and the test executables call it. A test built with looser flags than
the code it tests is a test that lies, and this keeps "the one flag set" true.

### Report back: C23 features used

**None.** `test.h` uses `static inline`, variadic-free function-like macros,
`__FILE__`/`__LINE__` and `fprintf` — C99 at the newest. No `nullptr`, no
`constexpr`, no `typeof`, no `[[attributes]]`, no C23 `static_assert`. This card
therefore puts no pressure on the Clang 18 floor. The float comparison computes
its own absolute difference rather than calling `fabs`, so tests do not link
libm either.

### Report back: the exact text of a failing check

Verbatim from the terminal, running the scratch `fail` test directly:

    FAIL  /home/ptcsoderlund/Projekt/voe3d_pre_study/voe3d/build/check/harness/tests/fail.c:5
          1 == 2
    FAIL  /home/ptcsoderlund/Projekt/voe3d_pre_study/voe3d/build/check/harness/tests/fail.c:6
          2 + 2 == 5
          actual:   4
          expected: 5
    FAIL  /home/ptcsoderlund/Projekt/voe3d_pre_study/voe3d/build/check/harness/tests/fail.c:7
          0.25 == 0.5
          actual:   0.25
          expected: 0.5
          off by 0.25, tolerance 1e-09

    exit code 3

Three failures, three exit codes: the run does not stop at the first. The path
is absolute because that is what `__FILE__` holds under Ninja; it is also what
lets an editor jump straight to the line.

### Verified

`cmake -P check.cmake` fails at step 1 on this machine — no `slangc` on PATH,
unchanged from card 001 and unrelated to this card. Re-run from a copy at the
repository root with one line changed, the `slangc -v` probe replaced by
`cmake -E true`; the copy was deleted and the real `check.cmake` never modified.
All ten steps `ok`, exit zero:

    ok    tools (clang 22, cmake 4.3.0, slangc)   <- probe substituted
    ok    standalone base
    ok    standalone math
    ok    root configure and build
    ok    guard compiler
    ok    guard version
    ok    guard map
    ok    includes
    ok    tests (the tree has none yet)
    ok    harness reports a failure

Step 6 says "the tree has none yet" because no folder has `tests/` yet. `ctest`
genuinely runs; it exits 8 with "No tests were found", which is read as a pass.
It is not a no-op, and (a) below is the proof.

### Each new check was made to fire

A check that only ever passes proves nothing, so each was broken on purpose:

- **(a) A failing test in the real tree fails step 6.** A temporary
  `math/tests/temp.c` with `VOE_TEST_CHECK(1 == 2)` gave
  `FAIL tests` / `1/1 Test #1: math/temp ***Failed`. This also shows the test
  name is `<folder>/<file>`, so `ctest -R math` selects one folder, and that
  step 2's standalone configure of `math` handles a folder that has tests.
  Removed afterwards.
- **(b) A harness that always reports green fails step 6b.** `voe_test_result()`
  was sabotaged to `return 0`. Step 6 still said ok — exactly the blind spot
  6b exists for — and 6b caught it:
  `FAIL harness reports a failure / a failing test reported success`. Reverted.
- **(c) A file outside `tests/` that includes `testing/` fails step 5.**
  A `math/include/math/probe.h` containing `#include <testing/test.h>` gave
  `FAIL includes / math/include/math/probe.h: only tests/ may include testing/`.
  Removed afterwards.

The harness scratch folder also proves the positive path: its `pass` test calls
into `voe_harness_value()` from the folder's own library, so a test linking both
`voe_<folder>` and `voe::testing` is exercised every run.

### One thing worth knowing

Putting the include in `math/src/` rather than a header fails at **step 3**, not
step 5: `src/` does not link `voe::testing`, so the compiler cannot find the
header and the build stops first. Step 5's rule is the belt to that compiler
error's braces — it still earns its place, because it names the file and the
rule instead of emitting a missing-header error, and it catches the case above
where nothing compiles the file at all.

### Not verified

- **Windows.** Linux only. Nothing here is platform-specific — CMake, `ctest`,
  and C `stdio` — and CMake handles the `.exe` suffix in `add_test`, but it has
  not been run there.
- `git` is still unusable in this checkout (`.git` is a gitlink to a
  `../.git/modules/voe3d` that does not exist), so this card was moved with `mv`
  and nothing was committed. Unchanged from the previous card.

### Suggestions, not done here

- `testing/` is not scanned by step 5, because step 5 walks discovered folders
  and `testing/` deliberately is not one. Nothing currently stops `test.h` from
  including an OS header. A small addition to step 5 could scan it by path.
- Step 6 reads "no tests found" as a pass, which is right today and wrong the
  day someone deletes every test file by accident. Once a folder has tests, a
  floor — "fail if fewer than N tests ran" — would close that.

Markers left in the code: none. No `DEVIATION:`, no `BLOCKED:`.
