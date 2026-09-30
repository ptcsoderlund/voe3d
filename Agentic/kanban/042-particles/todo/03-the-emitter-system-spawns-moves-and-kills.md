# 03 — The emitter system spawns, moves and kills
folder: 3d
after: 02
decisions: 0168, 0298

## Change
0298 points 2–4. Read `3d/include/3d/emitter_component.h` (card 02),
`3d/include/3d/shape_system.h` (a system that adds runtime rows to the
entities it owns), and `scene/include/scene/transform_system.h` for an
entity's world matrix.

- `3d/include/3d/emitter_system.h`, new:
  `void voe_3d_emitter_system_run(voe_ecs_world *world, float seconds)`.
  The header's points, in order of a run:
  - it drains the replace intents (a dead entity or one with no emitter is
    dropped; a texture with no NUL is ended and reported, as the model's);
  - it drains the controls: play sets playing and adds the row's burst,
    stop clears playing, burst adds `count` or the row's;
  - it adds a particles row to each emitter lacking one and drops rows whose
    emitter is gone, as the shape system adds its rows; a new row whose emitter
    plays takes the burst;
  - it spawns `rate × seconds` particles carried in the debt, plus the
    pending burst, up to 64 live; each is born at the offset through the
    world matrix with `speed` along a random direction within `spread` of
    `direction` through the world matrix's rotation, and a life of `life`;
  - it integrates: velocity gains `rise` up and loses `drag`; position
    moves; age grows; a particle past its life is removed by swapping the
    last in;
  - the random numbers are a xorshift in the row, seeded from the entity, so
    a run is the same each time;
  - `seconds` of 0 moves nothing but drains.
- `3d/src/emitter_system.c`, new: the run.
- `3d/tests/emitter_system.c`, new, one case per point:
  - 60 steps at 1/60 s of rate 20 life 10 give 20 particles, give or take one;
  - a burst of 10 on a stopped emitter gives 10 in one step;
  - stop keeps the live ones and adds none;
  - with rise 1 and speed 0, one step up is a positive y;
  - a particle is gone after its life;
  - never more than 64;
  - a removed emitter's particles row is gone after the next run.
- `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: one entry each.

## Done when
`ctest --test-dir build/debug -R '^3d/emitter_system$'` passes, after the
folder's build.
