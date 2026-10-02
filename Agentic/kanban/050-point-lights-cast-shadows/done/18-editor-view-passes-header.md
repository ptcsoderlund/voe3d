# 18 — The editor's view_passes.h header comment fits its cap
folder: editor/src
after: none
decisions: 0168

## Change
The header comment of `editor/src/view_passes.h` is 61 lines, cap 60. Keep
what the file does, how it is used and its constraints; move the why of any
one function to the comment above that function's declaration, or tighten
the prose, until the header is at most 60 lines. Change no declaration. If
`editor/src/src.md` repeats what moved, leave it as it is.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src`
reports no finding for `editor/src/view_passes.h`.
