# 14 — the game plays a scene with no light
folder: game
decisions: 0168, 0238, 0237

## Change
The game's frame needs no change of code: `voe_3d_draw_system_frame` stops asserting in card 11.
What changes is the recorded limit and the proof.

- `game/include/game/frame.h` — the Constraints line: exactly one camera and at most one light; a
  world with none draws every surface in its material colour, unshaded (0238). No other change.
- `game/tests/frame.c` — a second case beside the existing one: the same world built without the
  light (camera and cube only, light table still registered by `voe_game_world_new`), two frames
  through `voe_game_frame`, both return true, and the PNG is written and removed as the first case
  does. Its header gains the case and why (bug 01 of 024: a lightless scene asserted).
- `game/tests/tests.md` — the `frame.c` entry names the lightless case.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game` prints `FINDINGS: 0`.
2. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
3. The human's, from `bugs/01-a-scene-with-no-light-asserts.md`: open a project in the editor, delete
   the scene's Light entity; the views and the camera's corner picture show every shape in its own
   colour, flat. Press Play: the game opens and shows the same, with no assert. Add a light back
   (Add component, Rendering / Light) and the views shade again. Then walk `## How to test` in
   `feature.md` once more with the light in place.
