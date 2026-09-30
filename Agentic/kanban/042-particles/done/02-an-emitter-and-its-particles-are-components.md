# 02 — An emitter and its particles are components
folder: 3d
after: 01
decisions: 0168, 0298

## Change
0298 points 1–3. The types and intents only; the system is card 03.
Follow `3d/include/3d/model_component.h` and `3d/src/model_component.c` for
the shape of a described component with a replace intent, and
`3d/src/mesh_component.c` for a runtime-only type.

- `3d/include/3d/emitter_component.h`, new:
  - `VOE_3D_EMITTER_FIELDS(F, F_READ_ONLY)` with `VOE_BASE_DESCRIBE_STRUCT`:
    the fields of 0298 point 1, in that order and with those kinds (spread in
    degrees, colours COLOUR, texture CHAR of `VOE_3D_EMITTER_TEXTURE` = 128);
    `voe_3d_emitter_key`.
  - `voe_3d_emitter_intent {entity, emitter}`, the replace.
  - `voe_3d_emitter_control_kind` (PLAY, STOP, BURST) and
    `voe_3d_emitter_control {entity, kind, count}`.
  - `VOE_3D_EMITTER_PARTICLES` (64); `voe_3d_particle {position (double3),
    velocity, age, life}`; `voe_3d_particles {particles[64], count, debt,
    burst, seed}` and `voe_3d_particles_key`, runtime-only.
  - `voe_3d_emitter_register(world, capacity)`: the emitter with 0298's
    defaults, needing a transform, at "Rendering / Particle emitter", its
    intent as its replace; the control intent; the particles type with the
    same capacity.
  - `voe_3d_emitter_add(world, entity, emitter)`, a direct call as the
    model's; `_get`, `_count`, `_rows`, `_entities` for the emitter;
    `voe_3d_particles_get`, `_count`, `_rows`, `_entities`;
    `voe_3d_emitter_submit(world, intent)` and
    `voe_3d_emitter_control_submit(world, control)`, both returning false
    when the queue is full.
  - The header's points: what an emitter is and says; the defaults and why
    it plays at once (seen live in the editor); control stops spawning, not
    the particles; particles are world-space and runtime-only; the texture is
    a path that the model store loads (card 04), empty for the dot.
- `3d/src/emitter_component.c`, new: the keys, the registration, the reads
  and the submits.
- `3d/tests/emitter_component.c`, new: on a world with transforms
  registered, a row added with `voe_3d_emitter_add` reads back its fields;
  the default row reads 0298's defaults; both submits return true.
- `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: one entry each.

## Done when
`ctest --test-dir build/debug -R '^3d/emitter_component$'` passes, after the
folder's build.
