# 0382 — Windows trashes to the Recycle Bin through SHFileOperationW, on a fixed drive only
date: 2026-10-08
by: planner

## Decision
For 082 bug 02, carrying out 0381's "the Recycle Bin is Windows' trash" for `platform/trash.h`:
1. **The call** is `SHFileOperationW` with `FO_DELETE` and `FOF_ALLOWUNDO | FOF_NOCONFIRMATION |
   FOF_SILENT | FOF_NOERRORUI | FOF_WANTNUKEWARNING`, given the path made absolute by
   `GetFullPathNameW` and ended by two NULs. `shell32` is already linked by `platform` (0248).
2. **A drive without a Recycle Bin is refused** as `VOE_BASE_ERROR_UNSUPPORTED`, nothing deleted: the
   path's volume (`GetVolumePathNameW`) must be `DRIVE_FIXED` by `GetDriveTypeW`. This is 0378 point
   7's "another drive refused" on Windows. The nuke warning stays as the backstop for a fixed drive
   whose Recycle Bin is switched off, so nothing is ever deleted for good without a question.
3. **Failures**: nothing at the path, a path too long for the stack buffer, a non-zero return or an
   aborted operation are `VOE_BASE_ERROR_UNAVAILABLE`, the return code reported at the site.
4. **The test** checks only the nothing-is-unavailable case on Windows; it never sends anything to
   a person's real Recycle Bin.

## Reasoning
`SHFileOperationW` is one call in a library already linked and needs no COM; `IFileOperation` does the
same through COM in C for no gain here. With `FOF_ALLOWUNDO` and no UI, a drive with no Recycle Bin
(removable, network) deletes for good without asking, which Delete must never do; refusing anything
not fixed matches the Linux side's refusal of another file system. Writing the Recycle Bin's
`$I`/`$R` files by hand, as Linux writes the freedesktop trash, depends on an undocumented format.

## Replaces
nothing. Carries out 0381 for 0378 point 7.
