# 02 — Clouds, fog and sky say their ranges
folder: scene
after: 01
decisions: 0168, 0494, 0498

## Change
Give the weather components' bounded fields a range (`base/describe.h`'s ranges list, card 01).
Each header gets a `..._RANGES(R)` list beside its field list and its describe macro switches to the
ranged form; the struct, the scene text and every check stay as they are.

- `scene/include/scene/clouds_component.h`: `VOE_BASE_DESCRIBE_STRUCT_NAMED` becomes
  `VOE_BASE_DESCRIBE_STRUCT_NAMED_RANGED`. Ranges: `coverage` 0–1 and `shadow_strength` 0–1, unit
  empty; `wind_direction` 0–360, unit `°`.
- `scene/include/scene/fog_volume_component.h`: `VOE_BASE_DESCRIBE_STRUCT` becomes
  `VOE_BASE_DESCRIBE_STRUCT_RANGED`. Ranges: `thickness`, `patchiness`, `godrays_strength` 0–1,
  unit empty; `wind_direction` 0–360, unit `°`.
- `scene/include/scene/sky_component.h`: `VOE_BASE_DESCRIBE_STRUCT_RANGED`; `horizon_height` 0–1,
  unit empty.

Header point to add in each: the listed fields carry their range for a tool, which draws them as
sliders; wind direction is still accepted at any finite value, the range only says what a slider
offers.

Tests, one check each that `voe_base_ranges_find` on the description gives each listed field's ends
and unit, and NULL for one unranged number (clouds `height`, fog `wind_speed`, sky `strength`):
- `scene/tests/clouds.c`, `scene/tests/fog_volumes.c`, `scene/tests/sky.c`.

`scene/tests/tests.md`: each of those three entries also says its ranges. `scene/scene.md`: the
three component entries say "with their ranges" where they say "as a described field list".

## Done when
`ctest --test-dir build/debug -R '^scene/(clouds|fog_volumes|sky)$'` passes with the new checks.
