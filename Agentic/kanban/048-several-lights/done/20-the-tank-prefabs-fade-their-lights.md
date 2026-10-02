# 20 — The tank prefabs fade their lights
folder: examples/tank_game/Assets
after: 13, 18
decisions: 0168, 0272, 0321, 0322

## Change
The prefabs' lights lose the flash fields, gain a falloff, and carry the tank game's fade (0322
points 4 and 6). Read the header of `examples/tank_game/Code/tank_light_fade.h` and the files below.

- `examples/tank_game/Assets/enemy_tank.prefab`: `[2.voe_scene_point_light]` drops `flash` and
  `flash_when_made`, its `intensity` becomes 0 (it rests dark) and it gains `falloff = 1` after
  `range`; entity 2 gains a `[2.tank_light_fade]` section, `peak = 4`, `seconds = 0.12`,
  `left = 0`, placed just before its point light section.
- `examples/tank_game/Assets/enemy_wreck.prefab` and `examples/tank_game/Assets/house_wreck.prefab`:
  `[1.voe_scene_point_light]` drops the two flash keys and gains `falloff = 1` after `range`,
  intensity 6 kept; entity 1 gains a `[1.tank_light_fade]` section, `peak = 6`, `seconds = 0.6`,
  `left = 0.6` (a fade from full when spawned), placed just before its point light section.
- Every field written, numbers spelled as the neighbouring sections spell them. Nothing else
  changes.

## Done when
`(cd examples/tank_game/Assets && ! grep -n flash enemy_tank.prefab enemy_wreck.prefab
house_wreck.prefab && for f in enemy_tank.prefab enemy_wreck.prefab house_wreck.prefab; do
[ "$(grep -c '^falloff = 1$\|^\[[12]\.tank_light_fade\]$' $f)" = 2 ] || exit 1; done)` exits 0.
