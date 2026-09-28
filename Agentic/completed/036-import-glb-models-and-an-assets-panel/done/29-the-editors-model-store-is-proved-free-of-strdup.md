# 29 — The editor's model store is proved free of strdup
folder: editor/src
decisions: 0168

## Change
Bug 03, finishing card 28. Commit 70d07bc already replaced the `strdup(folder)` in
`voe_editor_models` (`editor/src/models.c`) with a `strlen` + `malloc` + `memcpy` copy; card 28
stopped only because `Agentic/tech-lead/system.md` was over its cap, and the map has since been
trimmed. Open `editor/src/models.c` and confirm the copy is `strlen(folder) + 1` bytes and keeps
the out-of-memory handling the file had. Change nothing if it does; otherwise fix it in that file
only. No other file.

## Done when
`! grep -rnw --include='*.c' --include='*.h' strdup base platform render 3d audio physics game
editor scene 2>/dev/null` exits 0, and `bash ~/.claude/skills/checks/scripts/checks.sh --all`
exits 0 and prints `FINDINGS: 0`.

For the human: on Windows, build `voe_editor` in CLion (debug preset, clang); it builds past
`editor/src/models.c`.
