# 054 — A quaternion has a length and can be normalised

claimed-by: -
blocked-by: -
status: todo
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
