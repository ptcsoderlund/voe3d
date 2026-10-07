# 12 — The platform headers' index lists trash.h
folder: platform/include/platform
after: none
decisions: 0168

## Change
`platform/include/platform/platform.md` has no entry for `trash.h`, which
card 03 added. Add one entry, after `file.h`'s or at the end beside its
kin: a file or folder sent to the freedesktop home trash, Linux only. Take
the point from the header comment of
`platform/include/platform/trash.h`; one sentence, under 300 characters.

## Done when
`bash /home/ptcsoderlund/.claude/skills/checks/scripts/checks.sh --folder platform/include/platform` prints `FINDINGS: 0`.
