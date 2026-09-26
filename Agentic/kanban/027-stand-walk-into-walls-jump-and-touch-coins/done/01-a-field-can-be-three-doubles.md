# 01 — A field can be three doubles
folder: base
decisions: 0168, 0250

## Change
0250 makes a transform's position three doubles; the description needs a kind for it. Only
`base` here: `authoring` and `editor` switch on the kind and are fixed by their own cards (06, 17).

- `base/include/base/describe.h` — a new kind `VOE_BASE_FIELD_DOUBLE3` after `FLOAT4X4` (no
  file stores a kind's number) and `VOE_BASE_FIELD_SIZE_DOUBLE3 24`. A comment on the kind: three doubles, a world position (0250),
  spelled in text the way FLOAT3 is, each element a FLOAT64.
- `base/tests/describe.c` — a struct with a DOUBLE3 field built through
  `VOE_BASE_DESCRIBE_STRUCT` (a local `struct { double x, y, z; }` type is enough): its row reads
  kind DOUBLE3, size 24, offset right; a DOUBLE3 array field `F(t, p, DOUBLE3, 2)` has count 2.
- `base/base.md` — the `describe.h` entry needs no change unless it lists kinds; check.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder base` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^base/describe$'` passes.
