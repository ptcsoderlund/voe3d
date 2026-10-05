# 23 — The render src docs fit their caps
folder: render/src
after: none
decisions: 0168

## Change
- `render/src/src.md`: the entry `pass.c` is 330 characters, cap 300. Make it one sentence under
  300; detail dropped and not already in `render/src/pass.c`'s header comment goes there.
- `render/src/descriptors.c`: its header comment is 61 lines, cap 60. Shorten it to 60 or fewer:
  tighten wording, or move the why of one function to the comment above that function.
Comments and the index only; no code changes.

## Done when
`checks.sh --folder render/src` prints no FINDING naming `src.md` or `descriptors.c`.
