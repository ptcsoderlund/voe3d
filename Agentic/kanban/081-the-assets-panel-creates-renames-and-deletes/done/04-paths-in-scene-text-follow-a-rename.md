# 04 — Paths in scene text follow a rename
folder: authoring
after: none
decisions: 0168, 0378

## Change
A renamed or moved asset is followed by every scene and prefab that names it, and Delete asks which do
(0378 point 1). Both are questions about scene text, so they are `authoring`'s; nothing here opens a
file.

- New `authoring/include/authoring/paths.h` with:
  - a struct `voe_authoring_paths_followed` — the rewritten `voe_authoring_text` (scene_write.h), how
    many values changed, and the longest rewritten value's length in bytes;
  - `[[nodiscard]] bool voe_authoring_paths_follow(const char *text, size_t size, const char *from,
    const char *to, voe_base_arena *arena, voe_authoring_paths_followed *out)` — every quoted string
    value that is exactly `from`, or begins with `from` and `/`, has that start replaced by `to`; every
    other byte copied as it was; false, reported, on a string never closed;
  - `bool voe_authoring_paths_named(const char *text, size_t size, const char *path)` — whether any
    quoted string value matches `path` the same way.
  Header points: a quoted string is one as scene_write.h writes it, alone after `=` or inside an array,
  with its `\"` and `\\` escapes stepped over and matched unescaped; keys, section names and unquoted
  values are never touched; a text rewrite, so kept sections and project components follow; a match is
  whole path segments, so `Assets/Rock` never matches `Assets/Rocks.glb`; `to` holds no quote or
  backslash (the caller's bug, asserted); the caller checks `longest` against its fields' room.
- New `authoring/src/paths.c` — one walk shared by both calls, no recursion.
- New `authoring/tests/paths.c` — `follow_renames_a_file`, `follow_moves_a_folders_contents`,
  `follow_leaves_a_longer_name_alone`, `follow_reaches_an_array_and_a_kept_section`,
  `follow_reports_the_longest`, `follow_refuses_an_open_string`, `named_finds_a_user`,
  `named_ignores_keys_and_names`.
- `authoring/authoring.md`, `authoring/src/src.md`, `authoring/tests/tests.md` — an entry each.

## Done when
`ctest --test-dir build/debug -R '^authoring/paths'` passes with the eight tests above.
