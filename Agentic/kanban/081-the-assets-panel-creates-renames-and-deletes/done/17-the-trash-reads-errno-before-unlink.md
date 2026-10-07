# 17 — The trash reads errno before cleaning up
folder: platform/src
after: none
decisions: 0168, 0378

## Change
One finding in this folder.

- `platform/src/trash_wayland.c`: the static analyser (clang-analyzer, run
  by `check.cmake`) reports at line 252 that `errno` is read after
  `unlink(info_path)` on line 251, which may change it. In the branch where
  `rename(path, file_path)` fails (line 248), save `errno` in a local before
  the `unlink`, and choose the returned error (EXDEV gives
  `VOE_BASE_ERROR_UNSUPPORTED`, as now) from that saved value. Look for any
  other place in the file that reads `errno` after a cleanup call and
  treat it the same way. No change to which error is returned for which
  failure.

## Done when
`bash /home/ptcsoderlund/.claude/skills/checks/scripts/checks.sh --folder platform/src` prints `FINDINGS: 0`, and
`ctest --test-dir build/debug -R '^platform/'` passes after a build.
