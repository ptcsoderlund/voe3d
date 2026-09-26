# 01 — A game may name ui and text
folder: cmake
decisions: 0168, 0259, 0175

## Change
- `cmake/voe.cmake`, `voe_allowed_deps` — the `game` row gains `text` and `ui` (after `render`,
  before `3d`). Its comment gains one point: text and ui for the project's interface drawn over
  the world (0259), and no theme because a game reads no project text.
- Nothing else. `game/CMakeLists.txt` is card 02's.

## Done when
1. `cmake --preset debug` exits 0.
2. `bash ~/.claude/skills/checks/scripts/checks.sh --folder cmake` prints `FINDINGS: 0`.
