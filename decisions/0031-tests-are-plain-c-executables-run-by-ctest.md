# 0031. Tests are plain C executables, one per module, run by CTest

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

D-011, and the last open Phase 2 decision. ADR-0028 defined `check.cmake`
step 6 as "tests pass" and left the mechanism here; the step is a no-op until
this ADR lands. The first `math` card needs it, because `voe_math_vec3_add` is
the first code in this project that can be wrong in a way the build will not
catch.

Constraints already fixed, and what each one eliminates:

- **Write it ourselves (ADR-0023).** A test framework is linked into a binary,
  so it sits on the ask-first side of the line. The default answer is to write
  it; the principal was asked and confirmed.
- **Every folder configures standalone (ADR-0001).** A standalone configure of
  `math` must build and run `math`'s tests. Test support cannot live behind the
  root.
- **`math` and `base` both depend on nothing (ADR-0022).** A shared test helper
  placed in `base` would create a `math -> base` edge. Test support therefore
  cannot live in an engine folder at all.
- **Four lines per folder (ADR-0027).** Tests must not add a fifth.
- **No `**` (ADR-0007, reaffirmed).** The classic self-registering test table is
  an array of function pointers walked through a pointer. Any design needing it
  is already rejected.
- **No new tool (ADR-0021).** CTest ships inside CMake, which is required
  already. Anything else is a fourth install.

A second consideration carries real weight here and is recorded because it is
not obvious from the constraints: **the primary author of these tests is an
agent working one card at a time**, with a small context budget. Four
mechanical properties follow from that, and they decided the option as much as
the constraints did.

1. Adding a test must not edit a shared file. A central registry listing every
   test function is an extra file in context, a conflict point between two
   concurrent cards, and a step that gets forgotten.
2. The failure output is the agent's whole context. It must name the
   expression, the expected value and the actual one, so the test source never
   has to be opened to fix a failure.
3. A folder's tests must be runnable alone, in seconds, with output scoped to
   the card.
4. There must be nothing to read in order to write a test.

## Options considered

### Option A — Plain C, one executable per test file, CTest, small own header
`<folder>/tests/<module>.c` is an ordinary C file with `main()`. `voe_module()`
globs `tests/*.c` and produces one executable per file, each registered with
`add_test()`. A header-only `voe::testing` target supplies the check macros and
the pass/fail epilogue. `check.cmake` step 6 becomes `ctest --output-on-failure`.

Costs one folder that is not engine code. Makes per-test CTest names, crash
isolation and `-R <folder>` filtering free, and needs no registration machinery,
so the `**` question never arises. Nothing about it is permanent: test files are
plain C and the header is replaceable by a mechanical edit.

### Option B — Vendor a single-header framework
greatest, utest.h or µnit — one file each, MIT, both platforms, same
executable-per-file shape and the same CTest wiring, but with suites, fixtures
and better failure formatting already written.

Costs the first third-party code in the tree, in the one component whose
failure mode is a false green. Also costs agent context: a framework arrives
with a README and an API surface, and an agent will read it before writing four
assertions.

### Option C — One executable per folder, self-registering, run by `check.cmake`
A registry of test functions per folder, a single `voe_<folder>_tests` binary
with a filter flag, and no CTest — `check.cmake` runs the binaries and parses
results.

Costs the registry file that every card must edit, the pointer-array idiom rule
6 pushes back on, whole-suite loss on one crash, and a results-parsing loop in
`check.cmake`. Buys a nicer standalone runner and nothing else.

## Decision

**Option A.** Deciding factor: CTest is already installed, which reduces B's
advantage over A to failure formatting — not a good trade for the project's
first dependency, in the component where a bug reports green.

The shape, fixed:

- **One test file per module, not per test case.** `math/tests/vec3.c` holds
  every `vec3` test. Three or four files per folder, not thirty.
- **`<folder>/tests/*.c`** — each file an ordinary translation unit with
  `main()`, globbed with `CONFIGURE_DEPENDS`, one executable per file. Adding a
  test creates one file and changes nothing else in the repository.
- **Target `voe_<folder>_test_<file>`; CTest name `<folder>/<file>`.** So
  `ctest -R math` runs exactly one folder's tests.
- **`testing/`** — a new top-level folder holding one header,
  `testing/include/testing/test.h`, exposed as the INTERFACE target
  `voe::testing`. Header-only: no `src/`, no `.c` file. It is **not** in the
  module map, is linked by test targets only, and never by a `voe_<folder>`
  library. `check.cmake` step 5's include grep enforces that: no file under any
  `src/` may include `testing/`.
- **The macros print expression, expected and actual.** That is the
  requirement, not a nicety — see consideration 2 above.
- **Zero, non-zero** is the pass/fail contract, so a test needs nothing from the
  header to be valid.
- **`check.cmake` step 6** is `ctest --output-on-failure` against the root
  `debug` build, and stops being a no-op.
- **No fifth line.** `voe_module()` adds the test targets when `tests/` exists;
  no `TESTS` argument, no per-folder opt-in.

## Blast radius

**Reversibility: cheap.** Test files are plain C with no framework in their
signatures; the header is a mechanical substitution and CTest registration is
inside one function. The only thing that would be tedious to reverse is the
one-file-per-module granularity, and only in proportion to how many test files
exist by then.

## Consequences

- **A twelfth top-level folder that is not engine code.** Accepted, and fenced:
  header-only, test targets only, and if it ever needs a `.c` file that is a
  card and a decision, not a quiet expansion.
- **Fixtures shared between two test files need a header in `tests/`.** Fine at
  this scale; if it stops being fine, that is evidence the module is too large.
- **More link steps than a single per-folder binary.** Trivial under Ninja at
  this size, and it buys crash isolation: a segfault fails one test, not a
  folder's suite.
- **Failure quality is now our responsibility.** A framework would have given it
  to us. The macros are small enough that this is a one-time cost, but it is a
  real one and it lands on the first `math` card.
- **`check.cmake` gets slower as tests accumulate.** Expected. The
  configure-only standalone step remains the dominant cost for now.
- **Nothing forces a folder to have tests.** `voe_module()` adds test targets
  when `tests/` exists and is silent when it does not. Coverage is a card's
  concern, not the build's.

## Rejected options and why

- **Option B** — pays the project's first third-party dependency for failure
  formatting, in the component whose failure mode is a false green, and imports
  an API surface an agent must read before writing a test.
- **Option C** — the registry file is a shared edit on every card and a conflict
  point between concurrent ones; the registration idiom is the banned
  pointer-array; and it replaces a tool we already have with a loop we would
  have to write.
- **A single test executable per folder with a CMake-generated `main`** —
  considered and dropped: it removes the shared registry but replaces it with
  codegen, which is one more rule an agent must know and one more thing that can
  fail confusingly.
- **Putting the test header in `base`** — creates `math -> base`, which is
  exactly the edge ADR-0022 forbids, for a reason that has nothing to do with
  the engine.

## Questions this opens

- **Closes D-011.** Exit criterion 7 keeps its remaining work: `check.cmake`
  passing on both platforms.
- **Dev executables are deferred, not rejected.** A `<folder>/dev/main.c`
  sandbox built automatically per folder was proposed and the principal chose to
  wait. New register row D-037.
- **D-034 is untouched.** `app` and tool executables still need
  `voe_executable()` or equivalent; test targets are added by `voe_module()` and
  do not answer it.
- The first `math` card is now unblocked and must state which C23 features it
  uses, for D-033.
