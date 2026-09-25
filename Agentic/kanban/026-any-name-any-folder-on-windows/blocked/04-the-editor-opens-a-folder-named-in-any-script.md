# 04 — The editor opens a folder named in any script
folder: editor
decisions: 0168, 0247, 0248

## Change
The editor reads its command line through card 03's `platform/include/platform/arguments.h`
(read its header). This is the last card of the feature.

- `editor/src/options.h` + `options.c`: `voe_editor_options_read` takes
  `voe_platform_arguments arguments` in place of `argc` and `argv`. The walk is the same over
  `arguments.count` and `arguments.values`. The header's rule 6 deviation paragraph goes, because
  it now lives in `arguments.h`, and it says the values are UTF-8.
- `editor/src/main.c`: `voe_platform_arguments_read(argc, argv, arena)`, then the options read, is
  done once the editor's arena exists (it is made just after the options read today). Nothing that
  opens a window or a device moves ahead of it. The header's mention of reading the command line
  stays true.
- `editor/src/src.md`: the `options.h` entry says the argument list is UTF-8 from `platform`.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. In `p=$(mktemp -d)/'Åsa 李 värld'` with `mkdir -p "$p" && cp -r examples/capsule/. "$p"`:
   `build/debug/editor/voe_editor --capture "$p/shot.png" "$p" 2>"$p/err"` exits 0, `$p/err` is
   empty, and `$p/shot.png` exists.
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's, on Windows: build the editor and follow steps 1–6 of `## How to test` in
   `feature.md`. Step 5 needs a second Windows user. If a step fails inside CMake, Ninja or
   clang, the build log says so; report which tool it was (0248).

## Blocked
The editor change is in and Done when 1 and 2 pass, but `checks.sh --all` prints `FINDINGS: 3`, all
in `platform` from cards 01–03: `platform/src/arguments_win32.c` line 22 (a `**` past one level of
dereference) and the `file.c` and `folder.c` entries in `platform/tests/tests.md` (350 and 305
characters, cap 300). A card for `platform` that fixes those three unblocks Done when 3; step 4 is
the human's on Windows.
