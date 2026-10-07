# 03 — A path goes to the desktop's trash
folder: platform
after: 02
decisions: 0168, 0378

## Change
Delete in the Assets panel sends a file or folder to the desktop's trash (0377 point 3, 0378 point 7).
Linux only (0339): a new header and a `_wayland.c` source, no Windows source.

- New `platform/include/platform/trash.h` —
  `[[nodiscard]] bool voe_platform_trash(const char *path, voe_base_arena *scratch, voe_base_error *error)`.
  Header points: the freedesktop.org trash specification's home trash, `$XDG_DATA_HOME/Trash` when that
  is absolute, else `$HOME/.local/share/Trash`, its `files/` and `info/` made as needed; the file or
  folder is renamed into `files/` beside an `info/<name>.trashinfo` holding `[Trash Info]`, `Path=` the
  absolute path percent-encoded, and `DeletionDate=` local time `YYYY-MM-DDThh:mm:ss`; a name already in
  the trash is numbered (`name.2`, `name.3`, ...), the info file made first with exclusive create so two
  callers never share a name; a path on another file system than the trash is
  `VOE_BASE_ERROR_UNSUPPORTED` and nothing moved (the per-drive `.Trash-<uid>` is not written); any other
  failure `VOE_BASE_ERROR_UNAVAILABLE`, reported at the site; on failure the info file is removed and
  the path is where it was; working memory from `scratch`, rewound.
- New `platform/src/trash_wayland.c` — carries it out with `realpath`, `open(O_CREAT | O_EXCL)`, `rename`
  and `localtime_r`; the folders made with `voe_platform_folder_create` (folder.h) one level at a time.
- New `platform/tests/trash.c` — sets `XDG_DATA_HOME` to a scratch folder and checks
  `trash_moves_a_file_with_its_info` (the file gone, in `files/`, its `.trashinfo` naming it),
  `trash_moves_a_folder_whole`, `trash_numbers_a_taken_name` and `trash_of_nothing_is_unavailable`.
- `platform/platform.md` gains the `trash.h` entry; `platform/src/src.md` the `trash_wayland.c` entry;
  `platform/tests/tests.md` the `trash.c` entry; each a phrase.

## Done when
`ctest --test-dir build/debug -R '^platform/trash'` passes with the four tests above.
