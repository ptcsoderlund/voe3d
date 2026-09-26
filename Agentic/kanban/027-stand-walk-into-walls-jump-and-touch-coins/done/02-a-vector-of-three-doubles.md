# 02 — A vector of three doubles
folder: math
decisions: 0168, 0250

## Change
A world position is three doubles (0250), named the way Slang names it.

- `math/include/math/double3.h` (new) — `voe_math_double3 { double x, y, z; }` and, each one line
  of arithmetic: `voe_math_double3_add(a, b)`, `voe_math_double3_sub(a, b)`,
  `voe_math_double3_from_float3(voe_math_float3)` (widen), `voe_math_double3_to_float3(a)`
  (narrow). Header points: why a position and nothing else is double (0250); that `to_float3` is
  meant for a difference of two nearby positions, which is small, never for a raw world position;
  why there is no length, scale or dot yet (rule 10: added when called).
- `math/src/double3.c` (new) — the four; `math/src/src.md` — its entry.
- `math/tests/double3.c` (new) — add/sub exact at 100000.25 + 0.001; a difference of two
  positions 100 km out narrowed to float keeps the millimetre; widen then narrow is the identity.
  `math/tests/tests.md` — its entry.
- `math/math.md` and `math/include/math/math.md` — the entry for `double3.h`.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder math` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^math/double3$'` passes.
