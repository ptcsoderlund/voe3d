# 20 — The editor src index entry fits its cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the entry `view_passes.h` is 310 characters, cap 300. Make it one sentence
under 300; detail dropped and not already in `editor/src/view_passes.h`'s header comment goes
there, comments only.

## Done when
`checks.sh --folder editor/src` prints no FINDING naming `src.md`.
