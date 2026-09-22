# Needs decision — check.cmake cannot run from a working tree on /mnt

## The question

Card 19 blocked on the only whole-suite finding: `cmake -P check.cmake` dies at
`standalone 3d` with `configure_file ... Operation not permitted` from CMake's own
`CMakeTestCCompiler.cmake`. This is not the feature's code. This working tree is
`/mnt/dev/GitRepos/Github/voe3d`, a 9p drvfs mount where every file reports
`root:root 0777` and uid 1000 cannot `chmod`, `utime` or `cp -p`; CMake preserves
permissions in `configure_file`, so any build tree under `/mnt` fails to configure
from scratch. Verified while planning: `chmod` on a fresh file there returns EPERM,
and `cmake -S /mnt/.../3d -B <ext4 dir> -G Ninja -DCMAKE_C_COMPILER=clang`
configures in under a second — the source on /mnt is fine, the build tree is not.

check.cmake puts its build trees in the tree (`set(checkdir "${root}/build/check")`,
line 39), and `cmake --preset debug` — the per-folder check in `CLAUDE.md` — writes
`build/debug`, which only still works because it was configured when the mount
behaved differently. So one of the two Checks is dead here and the other is not
reproducible, for every feature, not just 009. Where the tree lives and where the
verification harness builds reach the whole repository, so this is not the
planner's to settle.

## The options

1. **Work on ext4.** Move the working clone to a native path (`/` is ext4 with 996 GB
   free; the toolchain already lives natively in `~/voe3d-scratch`). No repository
   change; both Checks work as written, and check.cmake is reported to pass in ~110 s
   there. Costs: every agent, preset and cached absolute path that names
   `/mnt/dev/GitRepos/Github/voe3d` moves with it, and the Windows side loses direct
   filesystem access to the tree.
2. **Build outside the tree.** Teach `check.cmake` (and `CMakePresets.json`) to take
   their build directory from an environment variable with a default off the mount.
   Repository change, one file or two, and it is the smaller edit — but it only half
   fixes: `git`, ninja and the checks still run against 9p, and any future
   `configure_file` into the source tree fails the same way.
3. **Reduce the gate here.** Accept that `cmake -P check.cmake` is skipped on this
   machine and drop it from `## Checks`. Cheapest, and wrong: there is no CI
   (ADR-0004, ADR-0028), so this silently removes half the verification of every card.

## Recommendation

Option 1. The fault is the filesystem, not the harness; option 2 leaves a harness that
passes while the environment it claims to verify is still broken, and option 3 gives up
the only gate the project has. Once it is settled, `Agentic/kanban/009-editor-font/blocked/19-suite.md`
is simply re-run: it needs no code card unless `checks.sh --all` then reports something new.
