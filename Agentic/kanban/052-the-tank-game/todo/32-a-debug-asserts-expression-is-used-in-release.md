# 32 — A debug assert's expression is used in Release
folder: base
after: none
decisions: 0168, 0340

## Change
Bug 04: under `NDEBUG`, `VOE_BASE_DEBUG_ASSERT` puts its expression inside `sizeof`. Clang does not
count a `static` function named only there as used, so `is_finite` in
`scene/src/transform_component.c` fails the Release build with
`-Werror,-Wunneeded-internal-declaration`. The fault is the macro; no caller changes.

Read `base/include/base/assert.h`, `base/tests/tests.md`, the header comment of
`base/tests/samples.c` (the shape of a test) and `testing/include/testing/test.h`.

- `base/include/base/assert.h`: the `NDEBUG` shape of `VOE_BASE_DEBUG_ASSERT` puts the expression
  (still as a condition) and the message inside a branch that is never taken (`if (0)`), so both
  stay compiled and type-checked, neither runs, and Clang counts what they name as used. Checked
  with clang: `if (0)` passes `-Wall -Wextra -Werror` at `-O3 -DNDEBUG`, `sizeof` does not.
  The comment above it says so: why the branch and not `sizeof` (Clang's unneeded-declaration
  warning, 052 bug 04, ADR-0340), and that nothing in it runs. The file's top comment, which
  says the debug assert "still compiles under NDEBUG", stays true; adjust it only if it names
  `sizeof`. `VOE_BASE_ASSERT` and the Debug shape do not change.
- New `base/tests/debug_assert.c`: defines `NDEBUG` before any include, so the Release shape is
  tested in the Debug build. A `static` function, named only inside one `VOE_BASE_DEBUG_ASSERT`,
  counts its calls and returns false. The test passes when the program built (proof the function
  counts as used under `-Werror`), did not abort, and the count is 0 (the expression never ran).
  Its header says what it promises and why `NDEBUG` is defined inside the test (the suite and
  the folder check build Debug, so a test that followed the build would never see this shape).
- `base/tests/tests.md`: one entry for `debug_assert.c`.

## Done when
`ctest --test-dir build/debug -R '^base/debug_assert$'` passes, and
`cmake --preset release && cmake --build --preset release --target voe_scene` exits 0.
