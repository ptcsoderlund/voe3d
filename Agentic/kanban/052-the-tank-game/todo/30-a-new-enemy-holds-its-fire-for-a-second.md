# 30 — A new enemy holds its fire for a second
folder: examples/tank_game/Code
after: none
decisions: 0168, 0294, 0297, 0338

## Change
0338: the enemy's default `wait` becomes 1 s. Read `examples/tank_game/Code/tank_enemy.h`, the
header comment and the `tank_enemy_default` row of
`examples/tank_game/Code/tank_enemy_system.c`, and `examples/tank_game/Code/Code.md`.

- `tank_enemy_system.c`: `tank_enemy_default`'s `wait` is `1.0f`. The fire rule is unchanged.
  Its header's fire-rule paragraph says `wait` starts at the default 1 s, so a new enemy fires
  no sooner than 1 s after it appears, cites 0338.
- `tank_enemy.h`: the paragraph on `wait` says its default is 1 s, the hold before a new
  enemy's first shot (0338), while it turns and drives as ever.
- `Code.md`: the `tank_enemy.h` entry says the wait's default, 1 s; the entry stays one line.

No other file changes; `Assets/enemy_tank.prefab` and the scenes do not name `wait` and stay
untouched.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q '\.wait = 1\.0f' examples/tank_game/Code/tank_enemy_system.c` and
`! grep -qn '^wait' examples/tank_game/Assets/enemy_tank.prefab` each exit 0.

The human's, in the editor on `examples/tank_game` (Play), then in the shipped game from the menu:
1. Stand where a wave's enemy appears, in its line of fire. It turns and drives at once but does
   not fire for about the first second; then it fires as before.
2. Ship, run the shipped game, and see 1 the same.
