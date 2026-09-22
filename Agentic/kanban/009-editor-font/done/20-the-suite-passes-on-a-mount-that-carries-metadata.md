# 20 — The suite passes on a mount that carries metadata
folder: .
decisions: 0168, 0213

## Change
No repository change. Card 19's only finding was `cmake -P check.cmake` dying at
`standalone 3d` with `configure_file ... Operation not permitted`; ADR-0213 settles
that this is the `drvfs` mount under the tree, not the harness and not 009's code.
`/etc/fstab` line 2 mounts `C:\Users\PerSoderlund\Dev` on `/mnt/dev` with `defaults`,
which drops `metadata`, so every file reads `root:root 0777` and uid 1000 cannot
`chmod` — CMake's own compiler test fails and no build tree configures anywhere
under the tree. Repairing the mount is the human's step below; it needs root and
this account has no non-interactive `sudo`.

The coder's whole job is the probe and the run:

1. Probe the mount before anything else:
   `f=$(mktemp ./build/.permXXXX) && chmod 600 "$f" && echo METADATA_OK; rm -f "$f"`
   run from the repository root. If it prints `Operation not permitted`, stop and
   block this card with that line and "mount lacks metadata (ADR-0213)"; the human's
   step has not been done yet.
2. With the probe passing, run the suite and read what it reports.

If the suite then reports a finding that is 009's own code, do not fix it here:
block this card naming the folder and the finding, so it is planned as its own card.
ADR-0213 forbids the other repairs outright — `check.cmake`, `CMakePresets.json` and
`CLAUDE.md`'s `## Checks` are not to be edited, no build directory gains an escape
hatch off the tree, and the tree is not moved.

## Done when
- The probe in step 1 prints `METADATA_OK`.
- `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`
  (delete `.git/agentic-suite-green` first if it exists, so the suite really runs).
- `git status --porcelain` lists nothing outside `Agentic/`: this card changes no code.

For the human, once, on this machine, before the coder runs:
1. Edit `/etc/fstab` line 2 to read
   `C:\Users\PerSoderlund\Dev /mnt/dev drvfs defaults,metadata,uid=1000,gid=1000 0 0`.
2. Apply it: `wsl --shutdown` from Windows and start the distribution again (a
   `mount -o remount` on 9p drvfs is not reliable), then check with
   `mount | grep ' /mnt/dev '` that `metadata` is in the options and with
   `ls -l CLAUDE.md` in the tree that it is owned by the login user, not `root`.
3. `git config core.filemode` still prints `false` in this clone, so the mode bits
   the mount now reports show up in no diff.
