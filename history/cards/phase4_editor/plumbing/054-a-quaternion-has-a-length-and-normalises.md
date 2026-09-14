# 054 — A quaternion has a length and can be normalised

claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -
status: review
decision: *A drain corrects a bad value or keeps the last valid one, and says so* (ADR-0138) — the transform's drain normalises its rotation, so `math` gains a quaternion normalise on that card's demand (rule 10). Card 056 is the caller.

## Goal

`voe_math_quat_length` and `voe_math_quat_normalize` exist, spelled and behaving like their
`float4` counterparts.

## Scope

**1. `math/include/math/quat.h`, and the source file that defines `voe_math_quat_mul`.**

- `float voe_math_quat_length(voe_math_quat q)` — the square root of the sum of the four
  components squared.
- `voe_math_quat voe_math_quat_normalize(voe_math_quat q)` — `q` divided by its length.
  **It asserts on a zero length**, exactly as `voe_math_float4_normalize` does, for the
  reason `math/include/math/float4.h`'s header gives. Match that function's wording and its
  assert rather than inventing new ones.

**2. Tests — `math/tests/quat.c`.** A quaternion of length 2 normalises to length 1 within
the tolerance the file already uses; a unit quaternion comes back unchanged; normalising
`2·q` gives `q`, so the rotation is kept.

**3. `math/math.md`** — the `quat.h` entry mentions the two.

## What must not change

- No other function in `math`; no conversion between `quat` and `float4` added.
- No file outside `math` is edited. `math` depends on nothing.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R math` passes.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

Two functions with a caller already waiting on the board, tested the way the rest of `math`
is.

## Notes

**Verified on Linux, 2026-09-12.** `cmake -P check.cmake` green end to end — 41 tests
passed, `math` 5 of them, analyser clean over 110 files. `ctest -R math` 5/5. `math`
configures and builds standalone from a fresh build directory. From the planning root
`bash tools/hot.sh` reports `all hot files under their ceilings`; `math/math.md` is 35/120.

Windows was not checked — one machine, one platform (2026-09-04). Nothing here is
platform-specific: four floats, `sqrtf`, and an `assert`.

No `BLOCKED:` and no `DEVIATION:` markers. Five files changed, all in `math` plus this card.

**Two comments were corrected because this change falsified them**, not as repairs made on
the way past:

- `math/include/math/quat.h`'s *there is one way to build one* paragraph listed `no _mul, no
  _normalize, no _slerp`. It had to be rewritten to stop claiming `_normalize` is absent, and
  a rewrite could not honestly leave `no _mul` standing either — `_mul` has been declared
  directly beneath it since an earlier card. It now says what is present, why each arrived,
  and that `_identity` and `_slerp` are still waiting for a caller.
- `math/tests/quat.c`'s header argued that the matrix is the whole surface and that reading
  components back would assert the half-angle formula at itself. That is still right for
  direction and amount, and `check_normalize` is the exception — its claim *is* about the
  four components. The paragraph now says so.

`_normalize` divides by the length rather than multiplying by a reciprocal, which the card's
wording asks for and which keeps the local declarations at the top of the block.
`_length` writes the sum of squares out rather than going through a `_dot`, because this type
has none and rule 10 says not to add one.

**Suggestion, not done here:** `check_normalize` builds its length-2 quaternion by doubling
each component by hand, because `quat` has no `_scale`. If a later card gives it one, that
test gets shorter. Nothing calls for one today.
