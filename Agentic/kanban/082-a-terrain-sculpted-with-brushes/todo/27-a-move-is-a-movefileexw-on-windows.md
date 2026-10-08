# 27 — A move is a MoveFileExW on Windows
folder: platform
after: none
decisions: 0168, 0381, 0248, 0378

## Change
Bug 02: `voe_editor_assets_move` does not link on Windows because
`voe_platform_file_move` (`platform/include/platform/file.h`) has only its
Linux half, in `platform/src/file_wayland.c` (renameat2 without replace),
written under 0339, which 0381 replaces. Add the Windows half. Nothing outside
`platform` changes. `_win32.c` files are not built on Linux, so this code is
written to the header and the Linux side, not compiled here; write it as
carefully as code that is.

Files: `platform/src/file_win32.c`, `platform/src/src.md`,
`platform/include/platform/file.h` (header comment only, if a point below is
not already true there). Read `platform/src/file_wayland.c`'s
`voe_platform_file_move` and `platform/src/wide_win32.h` for the shape to
follow.

In `file_win32.c`, add `voe_platform_file_move(from, to, error)`:
- NULL `from` or `to` aborts (rule 13), as the Linux half does.
- Both paths converted through `wide_win32.h` into MAX_PATH stack buffers, as
  the other functions in the file do; one that does not fit is UNAVAILABLE,
  reported, nothing moved.
- `MoveFileExW(from, to, 0)`: no `MOVEFILE_REPLACE_EXISTING`, so a taken
  target fails, and no `MOVEFILE_COPY_ALLOWED`, so a move to another volume
  fails instead of copying. A folder moves whole on one volume.
- `ERROR_ALREADY_EXISTS` or `ERROR_FILE_EXISTS` is REFUSED; any other failure
  UNAVAILABLE; the GetLastError reason reported at the site through
  `base/report.h` as the rest of the file does.

The file's header comment gains the point that a move is MoveFileExW with no
flags and why each of the two flags is left out. `src.md`'s `file_win32.c`
entry names the move. `file.h` says MoveFileExW beside renameat2 where it
names the Linux call for a move, if it names one.

## Done when
- `grep -c "voe_platform_file_move" platform/src/file_win32.c` prints at least
  `1`, and `grep -n "MOVEFILE_REPLACE_EXISTING\|MOVEFILE_COPY_ALLOWED"
  platform/src/file_win32.c` shows neither flag on the move's call.
- `grep -n "move" platform/src/src.md` shows the `file_win32.c` entry naming it.
