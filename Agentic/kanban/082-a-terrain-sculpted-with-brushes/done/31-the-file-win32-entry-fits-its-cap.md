# 31 — The file_win32.c entry fits its cap
folder: platform/src
after: none
decisions: 0168

## Change
`platform/src/src.md`: the `file_win32.c` entry is 351 characters, cap 300.
Make it one sentence under the cap: what the file is (the Windows half of
`platform/file.h` through the Win32 file calls, a write made atomic and a move
that never replaces, both by MoveFileExW). Drop the "its header says why..."
tail. Then open `platform/src/file_win32.c`'s header comment only and make
sure the two points the entry drops are there (why a 64-bit count is moved in
steps; why a path too long for its stack buffer fails as an open); add any
that is missing. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder platform/src` prints
no finding naming `platform/src/src.md`.
