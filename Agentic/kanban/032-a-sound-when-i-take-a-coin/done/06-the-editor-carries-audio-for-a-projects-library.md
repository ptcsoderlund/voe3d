# 06 — The editor carries `audio` for a project's library
folder: editor
decisions: 0168, 0175, 0242, 0245, 0265, 0266

## Change
A project's library binds its engine calls to the editor's own exported symbols (0242 point 4,
0245); from card 08 on, the coin system calls `voe_audio_mixer_play`, so the editor must hold
`audio` whole or the library will not load. The editor plays nothing itself (0266 point 6).

- `cmake/voe.cmake` — `editor`'s row gains `audio`; its comment gains a clause: for a project
  library's symbols, not for sound of its own (0265, 0266).
- `editor/CMakeLists.txt` — `DEPENDS` gains `audio`.
- `editor/editor.md` — one phrase: it links `audio` so a project library that plays sounds
  loads.

Nothing in `editor/src/` changes: `voe_executable` already links every `DEPENDS` folder but
`game` whole-archive and writes the Windows `.def` from all of them.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. `nm -D --defined-only build/debug/editor/voe_editor | grep -c voe_audio_mixer_play` prints 1.
