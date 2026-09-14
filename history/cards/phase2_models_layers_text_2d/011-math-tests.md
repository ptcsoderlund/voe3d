# 011 — tests for `math`

status: complete
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -

The oldest open gap on the project. `math` shipped with no `tests/` folder, and
step 6 passes trivially when a folder has none — so a green check currently proves
nothing about any vector or matrix in the engine.

## Goal

`math/tests/` exists and `ctest` runs real assertions over everything `math`
actually has today.

## Scope — what exists, and nothing more

- `float3`, `float4`, `float4x4`, and whatever else is genuinely in the folder.
- **Storage layout.** ADR-0035 fixes `m[row][column]`, row-major, so that a GPU
  upload is a straight copy. Test it as a memory layout claim — write known values,
  assert the byte order. This is the CPU half and it is testable here.
- **Semantics stay column-vector `M * v`** with translation in the last column.
  Layout and semantics are different claims and both want asserting, because
  getting one right and the other wrong is the bug that looks like nothing.
- `_mul` is component-wise, not a dot product. `_scale` takes a scalar. Assert the
  spelling means what ADR-0035 says it means.

## Explicitly not this card

- **The rotation type.** It is still missing and the principal has decided it
  arrives with the camera card, when something actually rotates — nothing calls it
  yet, and `CLAUDE.md`'s implement-on-demand rule applies.
- **The `slangc` matrix-layout test** ADR-0035 asks for. That one needs a matrix
  sent to the GPU and compared, so it belongs to the first card that sends one.
  This card tests the CPU side only; say so in the test file's header so nobody
  thinks the flag is covered.
- Any function `math` does not have. Needing one is a finding to report, not a
  licence to write it.

## Verify

- `ctest --output-on-failure` runs them, and `check.cmake` exits zero.
- **Break one assertion on purpose and confirm the check fails.** Same reason as
  above.

## Notes — coder, 2026-09-01. Unfinished, and why

The tests are written and they pass. The card still cannot move, because
"`ctest` runs them, and `check.cmake` exits zero" cannot be reached from inside
`math/`.

### Done, in the tree

- `math/tests/float2.c`, `float3.c`, `float4.c`, `float4x4.c` — one per module,
  covering only what the folder has today. No rotation type, no `slangc` test.
- `math/math.md` — four lines added for them.

`float4x4.c` asserts the layout and the semantics as two separate claims, which
is what the card asked for: the sixteen floats are `memcpy`'d out and indexed
flat, so `m[row][col]` is proven to sit at `row * 4 + col` with no padding; and
separately, translation is proven to be the last column (flat 3, 7, 11) with the
last row still zero, composition is proven to apply the right-hand side first,
and `mul_float4` is checked against a matrix with sixteen distinct elements, so
multiplying by the transpose gives 90 where the test expects 30. Its header says
the `slangc -matrix-layout-row-major` half is not covered here.

### Was BLOCKED, now resolved with Human: libm, in `cmake/voe.cmake`

`voe_math_*_length` calls `sqrtf` and nothing linked `m`. Latent since `math` was
written — nothing had ever linked `voe_math` into an executable, `dev` depending
on `render platform base` — so the first test executable was the first thing to
hit it, on the coder's machine and then on Human's:

    ld.bfd: math/libvoe_math.a(float2.c.o): in function `voe_math_float2_length':
    math/src/float2.c:45: undefined reference to `sqrtf'

`math/CMakeLists.txt` is four lines by rule and has nowhere to say this, so this
was reported rather than made. Human chose the placement: `voe_target_settings()`
links `m` on Linux for every target, on the grounds that libm is the other half
of the C standard library on glibc and not a dependency in rule 5's sense. The
alternative considered and rejected was a named `voe_math_link()` beside
`voe_platform_backend()` and `voe_render_shaders()` — narrower, but every later
folder that calls `sinf` would have come back to the same file.

That is an edit outside this card's folder. It is in the diff because Human
directed it, and it is the only file outside `math/` that this card touches.

### Not a blocker: the coder could not run `check.cmake`, but Human can

`cmake -P check.cmake` failed at step 1 here with "slangc could not be run: no
such file or directory". That is the coding agent's sandbox and not the machine:
`slangc` is installed, at `$VULKAN_SDK/bin`, and Human's `which slangc` finds it.
The sandbox hides most of `$HOME`, so nothing outside this repository could be
seen or trusted from inside it.

So step 1 through step 7 are still unrun by anyone, and they are Human's to run.
What was verified is below, and it was verified by compiling and running the
tests directly.

### What was verified instead

- `math` configures and builds standalone (Clang 22.1.8, Ninja), and
  `ctest --output-on-failure` runs all four: `math/float2`, `math/float3`,
  `math/float4`, `math/float4x4`, 4 passed, 0 failed.
- `base` still configures, builds and passes standalone after the libm change,
  which is the check that it did not disturb a folder that does not need it.
- `dev` and `render` could not be configured from the coder's sandbox, which
  cannot see `slangc`. Human's own run reports `ok standalone dev` and
  `ok standalone render`.
- All four tests compiled and run by hand with `-lm`, same flags
  (`-std=c23 -Wall -Wextra -Wpedantic -Werror`): four exit codes of zero.
- Two assertions broken on purpose, as the card asks. Indexing the flat array
  column-major fails twelve of the sixteen layout checks; expecting
  left-to-right composition fails with `actual: 12, expected: 22`. The harness
  reports, it does not stop at the first failure, and the message names the
  expression.
- `clang --analyze -Xanalyzer -analyzer-output=text`, the step 7 invocation, over
  all four test files and all four sources: clean.

### Also unverifiable from the sandbox: git

Every git command failed here with "not a git repository": `.git` is a submodule
pointer to `../.git/modules/voe3d`, and the parent repository is above the
sandbox's view. Almost certainly fine on Human's machine — the same masking hid
the Vulkan SDK. Nothing was committed and no card was moved by git.

## Notes — coder (second pass), 2026-09-01

**Resolved. The card is complete and moves to `review/`.** The implementation was
complete after the first pass; this pass audited it independently rather than
trusting it, and found nothing to change. The one thing still outstanding was
never a coding task — `cmake -P check.cmake` had to exit zero, and it cannot be
run from the coding sandbox. Human ran it. It is green, and the result is at the
end of these notes.

### Audited, not assumed

- **Coverage is total at the name level.** Every function declared in
  `math/include/math/{float2,float3,float4,float4x4}.h` is called by the
  matching test file — compared set against set, not eyeballed. Nothing in the
  folder is untested and no test names a function the folder does not have,
  which is the card's "what exists, and nothing more" in both directions.
- **The arithmetic was re-derived by hand, not just re-run.** The `counting()`
  matrix really does give 30/70/110/150 for `M·(1,2,3,4)` and 90 for the
  transpose; `a × b` is `(-3, 6, -3)` and `x × y = +z`, so the right-handed
  claim is the one being asserted; the inverse coefficients
  `0.5 / 0.25 / 0.125` with offsets `-0.5 / +0.5 / -0.375` are the correct
  inverse of `T(1,-2,3)·S(2,4,8)`. The tests assert true things, which is a
  separate question from whether they pass.
- **`m[2][2] = 0` on an identity really is a zero row**, so the determinant
  comment is accurate and the case is the singular one it claims to be.

### Verified in this pass, on this machine

- `math` configures and builds standalone (Clang 22.1.8, Ninja, `-Werror`), and
  `ctest --output-on-failure` reports 4/4: `math/float2`, `float3`, `float4`,
  `float4x4`.
- `base` and `platform` also still configure standalone, so the libm change in
  `cmake/voe.cmake` has not disturbed the folders that do not need it.
- **The deliberate break, redone rather than taken on faith.** Indexing the flat
  array column-major — `(i % 4) * 4 + (i / 4) + 1` — fails 12 of the 16 layout
  checks, `ctest` reports the test as Failed, and the run does not stop at the
  first one. A sample message: `actual: 10, expected: 7`, with the file, the
  line and the expression as written. The file was restored from a copy and the
  suite re-run green afterwards, byte-identical to before.
- **Step 7's analyser is clean** over all eight files (`math/src/*.c` and
  `math/tests/*.c`), run with step 7's own invocation:
  `clang --analyze -Xanalyzer -analyzer-output=text`.
- **Step 5 was reasoned through and passes.** `testing/` has no `CMakeLists.txt`,
  so it is not a folder in step 5's sense, so the tests' `#include
  <testing/test.h>` cannot trip the "includes a folder it does not DEPENDS on"
  rule — and it is inside `math/tests/`, which is the one place that header is
  allowed. `math/CMakeLists.txt` is `voe_module(math)` with no `DEPENDS`, which
  is correct: the folder depends on nothing.

### Still not runnable here, unchanged from the first pass

`slangc` is absent from the sandbox — `$VULKAN_SDK` points at
`~/vulkan/1.4.357.1/x86_64`, which does not exist from inside it. So:

- `check.cmake` stops at step 1: *"VOE3D requires slangc."*
- `render` cannot even **configure** — `cmake/voe.cmake:306`, via
  `voe_render_shaders()`. That is worth recording beyond this card, because it
  means every card from 012 onward that touches `render` is uncompilable from
  the coding sandbox, not merely unverifiable.
- git is unusable: `.git` is a submodule pointer to `../.git/modules/voe3d`, and
  the parent repository is above the sandbox. Nothing was committed and no card
  was moved by git. If this card is moved, it is moved by hand.

### The gate is closed — Human's `check.cmake`, all fifteen steps

Run by Human on the real machine, since the coding sandbox can see neither
`slangc` nor the Vulkan SDK:

    ok    tools (clang 22, cmake 4.3.0, slangc, wayland-scanner)
    ok    standalone base
    ok    standalone dev
    ok    standalone math
    ok    standalone platform
    ok    standalone render
    ok    root configure and build
    ok    guard compiler
    ok    guard version
    ok    guard map
    ok    includes
    ok    tests (6 passed)
    ok    harness reports a failure
    ok    analyser (22 files)
    ok    analyser reports a finding

`tests (6 passed)` is the four this card added — `math/float2`, `float3`,
`float4`, `float4x4` — alongside the two that were already there. Before this
card, step 6 counted two and `math` contributed none of them, which was the gap
the card opened with.

Every bullet of **Verify** now holds: `ctest` runs them, `check.cmake` exits
zero, and a deliberately broken assertion was confirmed to fail the check and
then restored.

### One finding, recorded rather than acted on

`render` cannot be **configured** from the coding sandbox at all — not merely
built — because `voe_render_shaders()` requires `slangc` at configure time
(`cmake/voe.cmake:306`). Card 012 onward all touch `render`. That is a standing
constraint on what a coding agent can verify, not a defect in this card, and it
is noted here because 012 is the next card and will meet it immediately.
