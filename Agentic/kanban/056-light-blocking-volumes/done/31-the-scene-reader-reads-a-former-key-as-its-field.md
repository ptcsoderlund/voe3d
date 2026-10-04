# 31 — The scene reader reads a former key as its field
folder: authoring
after: 29, 30
decisions: 0168, 0324, 0352, 0353

## Change
0353 point 3, proven on the blocker: a scene saved before the rename opens with each blocker as it
was. Read the headers of `authoring/include/authoring/scene_read.h`, `authoring/src/scene_read.c`,
`authoring/src/scene_scratch.h`, the former names section of `ecs/include/ecs/component.h`,
`scene/include/scene/light_blocker_component.h`, and `authoring/tests/scene_read_unsaid.c` as the
model for the new test.

- `authoring/src/scene_read.c`, `read_section` (and `field_by_name` or a helper beside it): a
  component key naming no field is looked up in `voe_ecs_component_formerly` for the section's
  type; a match is read as that field, with no warning. When the section also names the field
  by its current name, the current name's value is kept whatever the order of the lines, and the
  former line is a warning naming both keys. A key matching neither is ignored and a warning, as
  now. A field reached by a former key is not a missing field. Header point.
- `authoring/include/authoring/scene_read.h`: the paragraph on keys and missing fields says a
  key may be a field's former name (0353) and which wins when both are given.
- `authoring/tests/scene_read_former.c` (new), registering identity, transform and light blocker
  as the model registers its types:
  1. a scene text with three blockers saved as `kind = 0`, `kind = 1`, `kind = 2` reads as
     `block` All, Fill and Direct;
  2. that world written back with `voe_authoring_scene_write` holds `block =` and no `kind =`;
  3. a section with `block = 2` before `kind = 1`, and one with them the other way, both read
     Direct;
  4. a blocker with neither key reads All.
  Header: what it proves and that the warnings on stderr are the report's.
- `authoring/authoring.md` (the scene_read.h entry names former names), `authoring/src/src.md`
  (scene_read.c), `authoring/tests/tests.md` (the new file). Each at most 300 characters.

## Done when
The tests `authoring/scene_read_former`, `authoring/scene_read`, `authoring/scene_read_unsaid` and
`authoring/prefab` pass after the folder's build.
