# 011 — tests for `math`

status: todo
claimed-by: -
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
