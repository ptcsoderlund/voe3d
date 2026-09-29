# 23 — The tank game proves prefabs and spawning
folder: examples
decisions: 0168, 0283, 0272
read: feature.md

## Change
Needs card 22. The last card of 038. Read `examples/tank_game/tank_game.md` and
`examples/tank_game/Code/Code.md`; change nothing unless a check below fails inside `examples`.
If one fails outside `examples`, block naming the folder and the finding.

For the human: `## How to test` in `feature.md`, all six steps, in the editor on
`examples/tank_game`. Step 1's tank is the scene's `tank_body` with `tank_head` under it; step 3
needs a second turret `.glb` imported to swap to; step 5's shells fire on the left button or
Space; step 6 is the editor's scene after Stop. Commit a scene or prefab the steps change only if
the sponsor asks.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. The six steps above are the human's.
