# 02 — A path is moved without replacing
folder: platform
after: 01
decisions: 0168, 0378

## Change
The Assets panel renames and moves files and folders, and must never replace what is already at the
target (0378 points 3 and 8). Linux only (0339): no Windows code.

- `platform/include/platform/file.h` — add
  `[[nodiscard]] bool voe_platform_file_move(const char *from, const char *to, voe_base_error *error)`.
  Its comment makes these points: a file or a folder, with all it holds, moves or is renamed in one step;
  it never replaces; `VOE_BASE_ERROR_REFUSED` is a target already taken; `VOE_BASE_ERROR_UNAVAILABLE`
  is anything else (nothing at `from`, a missing target folder, another file system, permission), the
  reason reported at the site as the rest of the file does; on failure nothing moved; NULL paths assert.
- `platform/src/file_wayland.c` — carry it out with `renameat2(AT_FDCWD, from, AT_FDCWD, to,
  RENAME_NOREPLACE)`; `EEXIST` and `ENOTEMPTY` are REFUSED. Its header gains the reason a move is
  renameat2 and not `rename`.
- `platform/tests/file.c` — add `move_renames_a_file`, `move_carries_a_folder_whole`,
  `move_refuses_a_taken_name` (both still there, unchanged) and `move_of_nothing_is_unavailable`, in the
  scratch place the file's other tests use.
- `platform/platform.md` (the `file.h` entry), `platform/src/src.md` (`file_wayland.c`) and
  `platform/tests/tests.md` (`file.c`) — each gains the move, as a phrase.

## Done when
`ctest --test-dir build/debug -R '^platform/file'` passes with the four new tests in it.
