# 04 — A body's slope limit says its range
folder: physics
after: 03
decisions: 0168, 0494, 0498

## Change
`physics/include/physics/body_component.h`: a `VOE_PHYSICS_BODY_RANGES(R)` list beside the field
list, and `VOE_BASE_DESCRIBE_STRUCT` becomes `VOE_BASE_DESCRIBE_STRUCT_RANGED` (base/describe.h,
card 01). `slope_limit` ranges 0 to π/2, unit `rad`; write π/2 as a named constant in this header
if none exists. Struct, scene text and checks unchanged. Header point: the slope limit carries its
range for a tool, which draws it as a slider; `step_height` has no top and stays a number box.

`physics/tests/body.c`: a check that `voe_base_ranges_find` on the body's description gives
`slope_limit`'s ends and unit, and NULL for `step_height`. `physics/tests/tests.md`: the `body.c`
entry also says the range. `physics/physics.md`: the `body_component.h` entry mentions the range.

## Done when
`ctest --test-dir build/debug -R '^physics/body$'` passes with the new check.
