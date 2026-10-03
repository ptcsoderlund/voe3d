# 06 — A spawner is a wave that wakes as it scrolls into view
folder: examples/tank_game/Code
after: 04, 05
decisions: 0168, 0272, 0334

## Change
0334 point 3. The sponsor places spawners in the level as waves; `main.scene` is theirs and
stays untouched (an absent field reads its default). Read `tank_spawner.h`,
`tank_spawner_system.c`, `tank_scroll.h` and `Code.md`.

- `examples/tank_game/Code/tank_spawner.h`: `F(uint32_t, count, UINT32)` after `most` and
  `F_READ_ONLY(uint32_t, made, UINT32)` after `wait`. Header: a spawner is a wave; it sleeps
  until the scroll's `top` reaches its world z, then spawns `count` in all, 0 never ending;
  `made` counts, written only by the system; with no scroll row it is awake.
- `examples/tank_game/Code/tank_spawner_system.c`: defaults `count` 4, `made` 0. A
  sleeping spawner (scroll row, its world z below `top`) neither counts down nor spawns. A
  done one (`count` above 0, `made` at `count`) spawns no more. Each spawn not refused adds
  one to `made`; the row written whole. The file header to match.
- `examples/tank_game/Code/Code.md`: the spawner entries say the wave.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `grep -q tank_scroll_get examples/tank_game/Code/tank_spawner_system.c` exits 0.
