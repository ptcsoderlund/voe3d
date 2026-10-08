# 62 — The descriptors header fits its cap
folder: render/src
after: none
decisions: 0168

## Change
- `render/src/descriptors.c`: its header comment is 60 lines or fewer, down from 61. Tighten the wording.
  Lose no claim. Any reasoning that belongs to one function goes above that function. Change no code.
- `render/src/src.md`: the `descriptors.c` entry, only if it repeats a sentence the header no longer has.

## Done when
`checks.sh --folder render/src` reports no finding in `render/src`.
