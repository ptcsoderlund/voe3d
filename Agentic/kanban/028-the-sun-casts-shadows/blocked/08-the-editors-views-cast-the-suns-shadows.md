# 08 — The editor's views cast the sun's shadows
folder: editor
decisions: 0168, 0258, 0238
read: feature.md

## Change
Both scene views and the camera preview draw shadows; this card finishes the feature. Read
`3d/include/3d/draw_system.h` for `voe_3d_draw_system_shadows` and the pass camera's third
member (card 06).

- `editor/src/view_passes.h` — `VOE_EDITOR_CAPACITIES`: `.shadow_size =
  VOE_3D_SHADOW_TEXELS`; `.passes` grows by `VOE_RENDER_SHADOW_CASCADES` per view and for the
  preview; `.objects` by `VOE_GAME_WORLD_MAX_DRAWN * VOE_RENDER_SHADOW_CASCADES` per view and for
  the preview. The header's capacity paragraph gains the shadow passes and their casters, and
  the opening lines say each view's pass is preceded by its shadow passes.
- `editor/src/view_passes.c` — in the preview and in each view: once the frame's view and eye
  are this view's own, `_shadows`, a false stopping the rest as a refused pass does; the pass
  camera gets `frame.shadow`.
- `editor/src/src.md` — `view_passes.h`/`.c` entries: shadow passes before each view's pass.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/capsule/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0 and `$p/err` is
   empty.
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: `feature.md`'s `## How to test` steps 1–9 on `examples/capsule/` — shadows in
   both views; turning the light swings them; the ledge shades the capsule; in Play the jump's
   shadow stays below and shrinks on landing; walking, edges stand still; far out distant things
   keep shadows and close up the edge is crisp; 100 km along X looks the same in editor and
   game; deleting the light unlights, undo restores; the game runs as smoothly as before.

## Blocked
The editor change is in and done-when 1 and 2 pass (`--folder editor` FINDINGS: 0; capture exits 0 with
empty stderr), but `checks.sh --all` prints one finding outside this folder: `game/tests/tests.md`'s
`frame.c` entry is 332 characters against a cap of 300, left by card 07 (8acd55a). Shortening that
entry in `game` (the rest moves to `frame.c`'s header) unblocks it; step 4 is the human's.
