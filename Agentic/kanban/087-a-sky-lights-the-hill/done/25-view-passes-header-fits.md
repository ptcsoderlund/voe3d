# 25 — The editor's view passes header fits its cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/view_passes.h`: its header comment is 61 lines, cap 60. Bring it to at most 60: the
why of one function moves to the comment above that function's declaration in the same file;
what the file does, how it is used and its constraints stay. Comments only; no declaration
changes. Fix `editor/src/src.md`'s `view_passes.h` entry only if it now contradicts the header.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints `FINDINGS: 0`.
