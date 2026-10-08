# 61 — The 3d source index entries fit
folder: 3d/src
after: none
decisions: 0168

## Change
- `3d/src/src.md`: the `shape_system.c` entry is one sentence of 300 characters or fewer (now 306).
  A point it drops goes into the header of `3d/src/shape_system.c` (22 lines) only if the header
  does not already make it. Change no code.

## Done when
`checks.sh --folder 3d/src` reports no finding in `3d/src`.
