# 19 — The tank game starts with a ground, a camera and a sun
folder: examples
decisions: 0168, 0272, 0251, 0277, 0244
read: feature.md

## Change
Needs every card before it. The project the sponsor imports into (0272); its level is the
sponsor's from here on (0251). Copy the spelling of `examples/coin_game/project.voe3d` and
`examples/coin_game/main.scene`.

- New `examples/tank_game/project.voe3d`, naming `main.scene`.
- New `examples/tank_game/main.scene`, three authored things: `Ground`, a cube shape, colour
  (0.35, 0.4, 0.3), transform position (0, −0.05, 0), scale (40, 0.1, 40), a box collider;
  `Camera`, lens as the coin game's, transform position (0, 20, 12), rotation turned about X
  looking down ≈ 59° (quaternion (−0.4924, 0, 0, 0.8704)); `Sun`, the coin game's light and
  transform rows.
- New `examples/tank_game/Assets/.gitkeep` (empty; `.gitignore` keeps it), so the folder the
  panel shows exists in a fresh clone.
- New `examples/tank_game/tank_game.md`: `# tank_game`, one sentence (0268's top-down tank
  game, grown by milestone), entries for `main.scene` (the sponsor's level) and `Assets` (the
  sponsor's `.glb` models, imported in the editor).
- `examples/examples.md`: a `tank_game` entry.

Then the whole feature is proved: `bash ~/.claude/skills/checks/scripts/checks.sh --all`.

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
