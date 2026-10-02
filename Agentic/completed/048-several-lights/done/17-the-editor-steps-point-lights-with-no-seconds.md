# 17 — The editor steps point lights with no seconds
folder: editor
after: 16
decisions: 0168, 0321, 0322

## Change
The editor's frame step calls the point light system as it now is (0322 point 5). The Inspector's
Falloff needs no editor code: the field is described (0322 point 3). Read the header of
`scene/include/scene/point_light_system.h` and the files below.

- `editor/src/world_step.c`: `voe_scene_point_light_system_run(world)`, no seconds; the header's
  first sentence says the point lights' replaces, not their flashes.
- `editor/src/src.md`: the world_step.c entry to match, at most 300 characters.

## Done when
The folder's build passes, and `grep -q "voe_scene_point_light_system_run(world)"
editor/src/world_step.c` exits 0.
