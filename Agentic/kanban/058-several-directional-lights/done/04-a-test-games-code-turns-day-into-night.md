# 04 — A test game's code turns day into night
folder: examples/sun_and_moon/Code
after: none
decisions: 0168, 0349

## Change
The code for `## How to test` step 4. The folder is new; create it with its `Code.md`. Model every
file on `examples/capsule/Code`. Read `examples/capsule/Code/coin.h`,
`examples/capsule/Code/coin_system.c`, `examples/capsule/Code/project.c` and
`examples/capsule/Code/Code.md`, and the headers of `game/include/game/project.h`,
`scene/include/scene/light_component.h` and `scene/include/scene/light_system.h`.

- `day_night.h`: the Day Night component, on a directional light's entity. Fields:
  - `day` and `night`: the light's intensity by day and by night;
  - `day_shadows` and `night_shadows`: whether it casts shadows by day and by night;
  - `period`: seconds for a whole day and night, default 20;
  - `clock`: seconds into the period, read-only.

  Also `day_night_register` and `day_night_system_run(world, seconds)`. Header points: what the
  component teaches (game code changes a light through its intent, the whole row), and its
  constraints.
- `day_night_system.c`: `clock` moves on by `seconds`, wrapping at `period`. Night is a weight n
  in [0, 1]: day for the first 0.3 of the period, a smoothstep up over the next 0.2, night for 0.3,
  and down over the last 0.2.
  - The light's intensity is the mix of `day` and `night` at n.
  - `cast_shadows` is `day_shadows` while n < 0.5, else `night_shadows`.
  - When either differs from the row, it submits the light row with both changed through
    `voe_scene_light_submit`. A row with no light is skipped.
- `project.c`: the four entry points as capsule's are. Register registers Day Night, the systems
  run Day Night with the step's seconds, after the move runs nothing, and the interface draws none.
- `Code.md`: one entry per file, each under 300 characters.

## Done when
`(for f in examples/sun_and_moon/Code/*.c; do clang -std=c23 -fsyntax-only
-DVOE_BASE_DESCRIPTIONS=1 $(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1;
done)` exits 0, and `grep -q voe_scene_light_submit examples/sun_and_moon/Code/day_night_system.c`
exits 0.
