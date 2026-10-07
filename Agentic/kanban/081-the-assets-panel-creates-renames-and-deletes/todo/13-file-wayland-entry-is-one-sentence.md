# 13 — The file_wayland.c entry is one sentence
folder: platform/src
after: none
decisions: 0168

## Change
In `platform/src/src.md` the entry `file_wayland.c` is 335 characters,
cap 300. Cut it to one sentence under 300 characters naming what the file
does (open, read/write, close, the atomic write's rename, the move). Any
detail cut that is not already in the header comment of
`platform/src/file_wayland.c` goes there instead; change no code.

## Done when
`bash /home/ptcsoderlund/.claude/skills/checks/scripts/checks.sh --folder platform/src` prints `FINDINGS: 0`.
