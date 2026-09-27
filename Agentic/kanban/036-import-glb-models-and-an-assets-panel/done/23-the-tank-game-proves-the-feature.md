# 23 — The tank game proves the feature
folder: examples
decisions: 0168, 0272, 0251, 0277, 0244
read: feature.md

## Change
Needs every card before it. `examples/tank_game/` (`project.voe3d`, `main.scene`,
`Assets/.gitkeep`, `tank_game.md`) and its entry in `examples/examples.md` are already in the
tree from the blocked card 19; cards 20–22 cleared the table-of-contents findings that stopped
the whole suite. Change nothing unless the proof below finds something in `examples/`; a finding
in another folder is a `## Blocked`, not an edit.

For the human, the feature's `## How to test`, all nine steps, in the editor on
`examples/tank_game` with a hull `.glb` with baked textures and a second model: the panel
shows `Assets/`; Import copies the hull in; a model copied in by the file manager appears; a
dragged hull stands lit, shadowed and textured; a click selects and outlines it, the Inspector
shows its path, a drop onto the Inspector swaps it and Ctrl+Z undoes; four hulls follow a
re-export within a second or two; save, close, reopen keeps them; Play and Ship show them; a
text file named `broken.glb` placed says it could not be read and nothing crashes. Commit the
imported models only if the sponsor asks.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0 and prints `FINDINGS: 0`, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. The nine steps above are the human's.
