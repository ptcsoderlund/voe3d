# 09 — Each explosion lights its surroundings for a moment
folder: examples
after: 07, 08
decisions: 0168, 0272, 0320

## Change
A wreck flashes when it is spawned, with no code: its prefab's light is flashed when made (0320
point 2). This card ends the feature; the human's steps below are `## How to test` in
`Agentic/kanban/048-several-lights/feature.md`. Read `scene/include/scene/point_light_component.h`
and the files below.

- `examples/tank_game/Assets/enemy_wreck.prefab` and `examples/tank_game/Assets/house_wreck.prefab`:
  entity 1 of each gains a `[1.voe_scene_point_light]` section, every field written: fire orange
  (about 1, 0.55, 0.15), intensity about 6, range about 10 m, flash about 0.6 s,
  `flash_when_made = true`. Placed before its `voe_scene_transform` section.
- `examples/tank_game/tank_game.md`: the Assets entry says the wrecks flash a light when they appear;
  the Code entry names the muzzle light. Each at most 300 characters.
- Nothing in `main.scene`: the dusk sun and the lamps are the human's, in the editor.

## Done when
`grep -c '^\[1\.voe_scene_point_light\]' examples/tank_game/Assets/enemy_wreck.prefab
examples/tank_game/Assets/house_wreck.prefab` prints a count of 1 for each, and
`git diff --quiet HEAD -- examples/tank_game/main.scene` exits 0.

The human's, in the editor on `examples/tank_game` (feature How to test):
1. Set the sun dim and orange, as at dusk; add a point light near the ground: a pool of light shows
   around it; change its colour, strength and reach and the pool changes at once.
2. Duplicate it until there are 100 lamps about the level; the editor stays smooth.
3. Parent a light to the tank's turret and drive or move the tank: the light moves with it.
4. Press Play: each shot flashes the ground around the barrel, and each explosion lights what is
   around it for a moment.
5. The lamps look the same in the game as in the editor.
