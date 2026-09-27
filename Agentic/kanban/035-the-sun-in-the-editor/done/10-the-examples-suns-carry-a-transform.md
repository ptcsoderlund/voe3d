# 10 — The examples' suns carry a transform
folder: examples
decisions: 0168, 0273, 0251

## Change
Needs card 01. A scene saved before 0273 spells the sun as a `direction`, which the reader now
ignores with a warning, and has no transform, so its sun would shine along −Z. Migrate the two
example scenes by hand so each looks as it did; change nothing else in them (the coin game's
level is the sponsor's, 0251).

- `examples/coin_game/main.scene`, entity `[2]` "Sun", and `examples/capsule/main.scene`,
  entity `[4]` "Sun": delete the `direction = …` line from the `voe_scene_light` section, and
  add a `[N.voe_scene_transform]` section after it with
  `position = [0, 8, 0]`, `rotation = [-0.36514837, 0.18257419, 0, 0.91287094]` (x, y, z, w:
  the shortest arc from −Z to the old direction (−1, −2, −2)/3) and `scale = [1, 1, 1]`.
  Spell it as the camera's transform section above it is spelled.
- `examples/coin_game/coin_game.md` and `examples/capsule/capsule.md` (if it exists): nothing,
  unless a line speaks of the sun's direction.

## Done when
`grep -rn "^direction" examples/*/main.scene` prints nothing, and
`grep -c "voe_scene_transform\]" examples/coin_game/main.scene examples/capsule/main.scene`
shows each file's count one higher than `git show HEAD:<file> | grep -c "voe_scene_transform\]"`.
