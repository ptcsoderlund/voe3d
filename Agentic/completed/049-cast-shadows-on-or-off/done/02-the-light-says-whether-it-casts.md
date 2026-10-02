# 02 — The light says whether it casts
folder: scene
after: 01
decisions: 0168, 0316, 0324

## Change
0324 points 1–3 for the light. Read `scene/include/scene/light_component.h`,
`scene/include/scene/light_system.h`, `scene/src/light_system.c`,
`scene/tests/light.c`, the unsaid-row pair in `ecs/include/ecs/component.h`,
`scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`.

- `light_component.h`: `VOE_SCENE_LIGHT_FIELDS` gains `cast_shadows`
  (`bool`, BOOL) after `bounces`. Header point: whether this light's
  shadows are drawn at all; false costs nothing (0316) and is a new light's;
  a file saved before the field reads true (0324); which things cast is
  their own flag (3d's shape and model).
- `light_system.h`: the register comment says the default row casts no
  shadows and that the unsaid row is the default row casting.
- `light_system.c`: the default row's `cast_shadows` is false; register sets
  the unsaid row, a copy of the default with `cast_shadows` true, through
  `voe_ecs_component_unsaid_set`. The validity check is unchanged.
- `scene/tests/light.c`: the default row casts no shadows; the unsaid row
  equals the default row but casts; an intent with `cast_shadows` true
  lands and one with false lands after it; with descriptions compiled in,
  the description names `cast_shadows` as BOOL. Fix any size or
  field-count check the new field breaks.
- `scene/scene.md` (light_component.h's entry gains cast shadows),
  `src/src.md` and `tests/tests.md` where their entries no longer say what
  the file does.

## Done when
`ctest --test-dir build/debug -R '^scene/light$'` passes with the four
cast-shadows checks.
