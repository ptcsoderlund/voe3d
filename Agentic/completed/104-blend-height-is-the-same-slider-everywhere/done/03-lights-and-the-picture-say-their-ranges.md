# 03 — Lights and the picture say their ranges
folder: scene
after: 02
decisions: 0168, 0494, 0498

## Change
As card 02 did for the weather, give the lights' and the picture's bounded fields a range. Each
header gets a `..._RANGES(R)` list and its `VOE_BASE_DESCRIBE_STRUCT_NAMED` becomes
`VOE_BASE_DESCRIBE_STRUCT_NAMED_RANGED`; struct, names, scene text and checks unchanged.

- `scene/include/scene/light_component.h`: `disc_size` 0–`VOE_SCENE_LIGHT_DISC_SIZE_MAX`, unit `°`;
  `glow_size` 0–1, unit empty.
- `scene/include/scene/point_light_component.h`: `falloff`
  `VOE_SCENE_POINT_LIGHT_FALLOFF_LEAST`–`VOE_SCENE_POINT_LIGHT_FALLOFF_MOST`, unit empty. The ranges
  list sits after the constants it names, as the macro is expanded where the struct is declared;
  move the two constants above the struct if they are below it.
- `scene/include/scene/picture_component.h`: `exposure` −`VOE_SCENE_PICTURE_EXPOSURE_MAX` to
  `VOE_SCENE_PICTURE_EXPOSURE_MAX`, unit `stops`.

Header point to add in each: the listed fields carry their range for a tool, which draws them as
sliders.

Tests, one check each that `voe_base_ranges_find` gives each listed field's ends and unit, and NULL
for one unranged number (sun `intensity`, point light `range`), the names still found:
- `scene/tests/light.c`, `scene/tests/point_light.c`, `scene/tests/picture.c`.

`scene/tests/tests.md`: each of those three entries also says its ranges. `scene/scene.md`: the
three component entries say "with their ranges" where they say "as a described field list".

## Done when
`ctest --test-dir build/debug -R '^scene/(light|point_light|picture)$'` passes with the new checks.
