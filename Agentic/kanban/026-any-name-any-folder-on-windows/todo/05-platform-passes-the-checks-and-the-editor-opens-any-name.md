# 05 — Platform passes the checks, and the editor opens a folder named in any script
folder: platform
decisions: 0168, 0247, 0248

## Change
Card 04's editor change is already committed (editor reads its command line through
`platform/arguments.h`). What stops the feature are three `checks.sh --all` findings left in
`platform` by cards 01–03. This card fixes them and is the last card of the feature.

- `platform/src/arguments_win32.c`: line 22's `const char **values` trips the one-level
  dereference rule. Give one UTF-8 argument a file-local type name (a typedef of `const char *`,
  named for what it is), and make `values` a pointer to it, as `LPWSTR *wide` already reads. The
  header's rule 6 deviation paragraph stays; it adds that the local name is how the list is spelt.
- `platform/tests/tests.md`: the `file.c` entry (350 characters) and the `folder.c` entry (305)
  each become one sentence under 300 characters: what the file proves, in brief.
- `platform/tests/file.c` and `platform/tests/folder.c`: header only. The opening paragraph of
  each already lists its promises; add the one the index entry drops, that a non-ASCII name is
  written, found and read back (`file.c`) and that non-ASCII names list byte for byte
  (`folder.c`), if it is not already said. No code change.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` prints `FINDINGS: 0`.
2. In `p=$(mktemp -d)/'Åsa 李 värld'` with `mkdir -p "$p" && cp -r examples/capsule/. "$p"`:
   `build/debug/editor/voe_editor --capture "$p/shot.png" "$p" 2>"$p/err"` exits 0, `$p/err` is
   empty, and `$p/shot.png` exists.
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's, on Windows: build the editor and follow steps 1–6 of `## How to test` in
   `feature.md`. Step 5 needs a second Windows user. If a step fails inside CMake, Ninja or
   clang, the build log says so; report which tool it was (0248).
