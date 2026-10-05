# 09 — The editor's view.h entry and view_passes.h header fit their caps
folder: editor/src
after: none
decisions: 0168

## Change
Two documentation fixes, no code:

- `editor/src/src.md`: the entry for `view.h` is 332 characters, cap 300.
  Shorten it to one sentence under 300 (a scene view: its orbit, target,
  input, the view under the pointer, its colours, and the world camera's
  preview). Read the header comment of `editor/src/view.h`; any detail the
  entry drops that the header does not say goes into that header.
- `editor/src/view_passes.h`: its header comment is 64 lines, cap 60. Bring
  it to 60 or fewer without losing a fact: rewrap the over-long lines and
  merge the short ones (the `point 7, 0326 ...` line runs past the margin,
  the `room / for views` lines break early), or move the paragraph on
  VOE_EDITOR_CAPACITIES' transient numbers to the comment above
  VOE_EDITOR_CAPACITIES in the same file.

Only `src.md`, `view.h` and `view_passes.h` change.

## Done when
`checks.sh --folder editor/src` prints no finding for `src.md`, `view.h` or
`view_passes.h`.
