# 20 — A wreck that carries Tank / Fade away lies still, fades and goes
folder: examples/tank_game/Code
after: 16
decisions: 0168, 0336

## Change
0336 point 4, the code; the prefab is card 21. Read, in this folder, `tank_light_fade.h` and
`tank_light_fade_system.c` (a game component writing an engine row through its intent: the
pattern), `tank_enemy.h` for `TANK_ENEMY_ROWS`, `project.c`, `Code.md`, and the headers of
`3d/include/3d/model_component.h` (`fade`, `voe_3d_model_submit`),
`scene/include/scene/parent_component.h` (`voe_scene_parent_tree`) and
`game/include/game/project.h` (`voe_game_project_remove`).

- `tank_fade_away.h`, new: `F(float, wait, FLOAT32)`, `F(float, seconds, FLOAT32)`,
  `F_READ_ONLY(float, age, FLOAT32)` described as `tank_fade_away`, its key,
  `tank_fade_away_register(world)` and `tank_fade_away_system_run(step)`. Header: a thing that
  lies still `wait` seconds, then turns see-through over `seconds`, every model on it and under
  it, then leaves the world with its tree; `age` written only by the system; game code, through
  the model's fade (0336); what a full queue does.
- `tank_fade_away_system.c`, new: registers under "Tank / Fade away", defaults `wait` 2,
  `seconds` 1, `age` 0, capacity `TANK_ENEMY_ROWS` (a wreck lives seconds; the enemies bound
  them), no need. Each step each row's `age` grows by the step, written whole. The fade is
  (age − wait) / seconds clamped to 0..1, 1 for `seconds` 0 or less; each model row on the
  entity's tree (`voe_scene_parent_tree` into a local array, its cap a named constant) whose
  `fade` differs is submitted whole with it. At `age` ≥ wait + seconds the entity is removed by
  `voe_game_project_remove`; refused, it tries next step. A full model queue loses that step's
  fade, not the count.
- `project.c`: registers it; while playing, it runs after the enemy. The header's order,
  register list and constraints line to match.
- `Code.md`: entries for both new files; the project entry's count and order.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `grep -q tank_fade_away_system_run examples/tank_game/Code/project.c` exits 0.
