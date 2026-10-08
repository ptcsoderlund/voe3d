# 28 — The trash is the Recycle Bin on Windows
folder: platform
after: 27
decisions: 0168, 0381, 0382, 0378, 0248

## Change
Bug 02: `voe_editor_assets_trash` does not link on Windows because
`voe_platform_trash` (`platform/include/platform/trash.h`) has only
`platform/src/trash_wayland.c`. Add the Windows half as 0382 says. `_win32.c`
files are not built on Linux; write it as carefully as code that is.
`shell32` is already linked for `platform` on WIN32 (`cmake/voe.cmake`); no
CMake edit.

Files: new `platform/src/trash_win32.c`; `platform/include/platform/trash.h`,
`platform/src/trash_wayland.c` (header comments only), `platform/tests/trash.c`,
`platform/platform.md`, `platform/src/src.md`, `platform/tests/tests.md`. Read
`platform/src/wide_win32.h` and the header of `platform/src/file_win32.c` for
the Windows file shape.

`trash_win32.c`, `voe_platform_trash(path, scratch, error)`:
- NULL `path` or `scratch` aborts (rule 13); `scratch` is rewound before
  returning, as the header promises.
- `path` converted through `wide_win32.h`, nothing there
  (`GetFileAttributesW`) is UNAVAILABLE.
- Absolute by `GetFullPathNameW` into a buffer one wider than MAX_PATH, so
  the second NUL `SHFileOperationW` needs fits; too long is UNAVAILABLE.
- The volume by `GetVolumePathNameW`; not `DRIVE_FIXED` by `GetDriveTypeW` is
  UNSUPPORTED, nothing deleted.
- `SHFileOperationW`, `FO_DELETE`, the flags 0382 point 1 lists. A non-zero
  return or `fAnyOperationsAborted` is UNAVAILABLE, the return code reported
  through `base/report.h`.
- Its header comment: the Windows half of trash.h; why SHFileOperationW and
  not IFileOperation or hand-written `$I`/`$R` files; why only a fixed drive;
  why the nuke warning stays; why the double NUL.

`trash.h`'s header: the trash is the desktop's, the freedesktop home trash on
Linux and the Recycle Bin on Windows (0381, 0382); "Linux only (ADR-0339)"
goes; the Linux paragraphs are marked as Linux's; FAILURES says what
UNSUPPORTED and UNAVAILABLE are on Windows. `trash_wayland.c`'s header loses
"There is no _win32 half" and names `trash_win32.c` instead.

`tests/trash.c`: the freedesktop checks and their scratch setup are Linux's,
inside `#ifndef _WIN32`; on Windows `main` runs only
`trash_of_nothing_is_unavailable`, so the test never fills a real Recycle Bin
(0382 point 4). Its header drops "Linux only by construction (ADR-0339)" and
says this instead.

Index entries: `platform.md`'s `trash.h` line names both trashes; `src.md`
gains a `trash_win32.c` entry (SHFileOperationW into the Recycle Bin, a fixed
drive only) beside `trash_wayland.c`; `tests.md`'s `trash.c` line says the
Windows run checks only a missing path.

## Done when
- `test -f platform/src/trash_win32.c` and
  `grep -c "SHFileOperationW\|DRIVE_FIXED" platform/src/trash_win32.c` prints
  at least `2`.
- `grep -rn "0339" platform` prints nothing.
- `grep -n "trash_win32" platform/src/src.md platform/src/trash_wayland.c`
  shows both.
