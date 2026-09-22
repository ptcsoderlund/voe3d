# 0213 — The tree stays on the Windows drive and the mount carries metadata
date: 2026-09-22
by: tech-lead

## Decision
The working tree stays where it is, at `/mnt/dev/GitRepos/Github/voe3d`, which is
`C:\Users\PerSoderlund\Dev` seen from WSL. The human works on it from CLion on
Windows, so a native Linux path is not available to this product. The `drvfs`
mount that carries it must be mounted with `metadata,uid=1000,gid=1000`; without
those, every file reports `root:root 0777`, uid 1000 cannot `chmod`, and CMake's
own compiler test fails its `configure_file` with `Operation not permitted`, so
no build tree can be configured from scratch anywhere in or under the tree. Both
Checks in `CLAUDE.md` stay exactly as they are and keep building inside the tree:
`cmake -P check.cmake` in `build/check`, `cmake --preset debug` in `build/debug`.
Neither check.cmake nor `CMakePresets.json` gains an escape hatch for the build
directory, and neither Check is dropped or weakened. If a Check ever dies again
at `CMakeTestCCompiler.cmake` with `Operation not permitted`, the fault is the
mount, not the harness or the card: the mount options are the thing to look at
first, and the tree is not to be moved or the gate reduced in response.

## Reasoning
The fault was never the repository. `/etc/fstab` mounted `/mnt/dev` with
`defaults`, which omits `metadata`; before the VM's reboot on 2026-09-21 the
mount came from WSL's automount, which passed it. One flag went missing on one
boot and took half the verification with it, so the repair belongs in the mount,
where the breakage is, and costs the product nothing. Moving the clone to ext4
would have worked and needed no repository change, but it takes the tree away
from CLion, which is where the human actually works — that alone rules it out.
Teaching check.cmake and the presets to build outside the tree is a smaller edit
but a worse one: it leaves a harness reporting green while the environment it
claims to verify is still broken, and the next `configure_file` into the source
tree fails identically. Skipping `cmake -P check.cmake` here was cheapest and
wrong, because there is no CI (ADR-0004, ADR-0028) and it is half of the only
gate the project has. `core.filemode` is already `false` in this clone, so the
mode bits changing under `metadata` show up in no diff.

## Replaces
nothing.
