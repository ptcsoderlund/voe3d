# 0383 — A move checks then renames where the filesystem refuses no-replace
date: 2026-10-08
by: planner

## Decision
For 082: `voe_platform_file_move` on Linux stays `renameat2` with `RENAME_NOREPLACE`. When that call
fails with `EINVAL` (the filesystem does not support the flag; WSL's 9p `drvfs` mount, where the tree
lives under 0213, is one), the move falls back once to `lstat` on the target, REFUSED if anything is
there, else plain `rename`. Every other errno keeps its meaning; the fallback does not retry on them.

## Reasoning
The sponsor's tree and its `Assets/` live on `/mnt/dev`, a 9p mount that rejects `RENAME_NOREPLACE`
with `EINVAL` even when the target is free, so without a fallback every Assets-panel move fails there
and `platform/file`'s move test fails in the suite. The check-then-rename window is the race the
file's header rejects, accepted only on such a filesystem: nothing else writes a project's files while
the editor has it open.
- `link` then `unlink`: atomic for a file, but a folder cannot be linked, so two paths for one call.
- Refuse the move on such a filesystem: the editor cannot move assets in the sponsor's own tree.

## Replaces
Nothing. Narrows 0378's "a move is renameat2 without replace" for filesystems that lack the flag.
