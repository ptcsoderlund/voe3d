# 18 — The tank game fades a light by its own code
folder: examples/tank_game/Code
after: 13, 16
decisions: 0168, 0272, 0321, 0322

## Change
The tank game's own fade of a point light's intensity, which its shots and wrecks will use (0322
point 6). Read `examples/tank_game/Code/tank_gun.h` and `examples/tank_game/Code/tank_turret.h` as
the pattern (a described component, its registration, a call for others), the headers of
`scene/include/scene/point_light_component.h`, `scene/include/scene/point_light_system.h` and
`game/include/game/project.h`, and the files below.

- `examples/tank_game/Code/tank_light_fade.h` (new): `TANK_LIGHT_FADE_FIELDS`: `peak` FLOAT32 (the
  intensity at full, default 4), `seconds` FLOAT32 (how long it fades, default 0.12), `left`
  FLOAT32 (seconds still to fade, default 0); `tank_light_fade_key`;
  `[[nodiscard]] bool tank_light_fade_register(voe_ecs_world *world)` under "Tank / Light fade",
  needing a point light; `bool tank_light_fade_start(voe_ecs_world *world, voe_ecs_entity entity)`:
  `left` becomes `seconds`, false with no row; `void tank_light_fade_system_run(const
  voe_game_project_step *step)`. Header points: why game code and not the engine (0321 point 2);
  a `left` written in a prefab is a fade from full when it is spawned; the light shows
  `peak × left / seconds` this step, then `left` counts down by the step, never below 0; a light
  is set only through its replace, and only when its intensity differs, so a resting lamp costs no
  intent; a full queue skips that step's fade; a `seconds` of 0 or less is dark.
- `examples/tank_game/Code/tank_light_fade_system.c` (new, header comment): the key, the
  registration, the start, the run. Its own row is written whole through
  `voe_ecs_component_set`, as the gun's `wait` is.
- `examples/tank_game/Code/project.c`: registers it (twelve types) and runs it after the enemy and
  before the camera, so a shot fired this step shows full this step; the header's order and its
  Constraints line name it.
- `examples/tank_game/Code/Code.md`: one entry each for the two new files; the project.c entry
  says twelve types and the new order. Each at most 300 characters.

## Done when
`(for f in examples/tank_game/Code/tank_light_fade_system.c examples/tank_game/Code/project.c; do
clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1 $(for d in */include; do printf -- "-I%s " $d;
done) "$f" || exit 1; done)` exits 0.
