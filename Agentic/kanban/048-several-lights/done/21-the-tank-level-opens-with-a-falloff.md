# 21 — The tank level opens with a falloff
folder: examples/tank_game
after: 17, 19, 20
decisions: 0168, 0321, 0322

## Change
The level's saved lamp opens with no warning, and the bug is closed (0322 point 4). This card ends
048 bug 01; the human's steps below are its How to reproduce.

- `examples/tank_game/main.scene`: `[20.voe_scene_point_light]` drops `flash` and
  `flash_when_made` and gains `falloff = 1` after `range`. Nothing else in the file changes: the
  level is the sponsor's.
- `examples/tank_game/tank_game.md`: the Assets entry says the wrecks fade a light as they appear;
  the Code entry names the light fade. Each at most 300 characters.

## Done when
`! grep -n "flash" examples/tank_game/main.scene` exits 0, `grep -c "^falloff = 1$"
examples/tank_game/main.scene` prints 1.

The human's, in the editor on `examples/tank_game` at dusk (a dim orange sun):
1. Select the lamp or add a point light near the ground: the Inspector shows Colour, Intensity,
   Range and Falloff, and no Flash or Flash when made; the lamp looks as it did before.
2. Drag Falloff down: the pool turns even with a soft rim. Drag it up: the light gathers at the
   lamp. The pool ends in the same place throughout; undo brings the old falloff back.
3. Select the sun: it has no Falloff.
4. Play: each shot flashes the ground around the barrel, each explosion lights its surroundings
   for a moment.
5. The lamps look the same in the game as in the editor, at any falloff.
