# 32 — A move falls back where the filesystem refuses no-replace
folder: platform/src
after: 31
decisions: 0168, 0383, 0378

## Change
`platform/file` fails in the suite: on the 9p `drvfs` mount at `/mnt/dev`,
where the tree and its build live, `renameat2(..., RENAME_NOREPLACE)` returns
`EINVAL` even when the target is free ("moving ... failed: Invalid
argument"), so every move fails there. With a taken target the mount still
returns `EEXIST`, which already maps right.

File: `platform/src/file_wayland.c`, function `voe_platform_file_move` only.
Per 0383: when `renameat2` fails with `EINVAL`, fall back once to `lstat` on
`to`; if it succeeds, the target is taken and the result is what an
`EEXIST` gives today (REFUSED, reported the same way); if it fails with
`ENOENT`, call plain `rename(from, to)` and map its errno through the same
mapping the `renameat2` failure uses; any other `lstat` errno is reported
and mapped as an ordinary failure. Every errno other than `EINVAL` from
`renameat2` keeps its current path. Read errno right after each call, before
any reporting.

Header comment of `file_wayland.c`: the "A MOVE IS renameat2 WITH
RENAME_NOREPLACE" paragraph gains the points that a filesystem without the
flag answers EINVAL (WSL's drvfs, where the tree lives), that the move then
checks and renames, and that the window this opens is accepted only there
(0383). `<sys/stat.h>` if `lstat` is not declared yet.

No other file: `platform/include/platform/file.h`, `file_win32.c` and the
tests stay as they are; the existing move cases in `platform/tests/file.c`
are the proof.

## Done when
- `grep -n "EINVAL" platform/src/file_wayland.c` shows the fallback in
  `voe_platform_file_move`.
- `bash ~/.claude/skills/checks/scripts/checks.sh --folder platform/src`
  prints FINDINGS: 0 with the build under `/mnt/dev`, which runs the test
  `platform/file` on the 9p mount; it passes.
