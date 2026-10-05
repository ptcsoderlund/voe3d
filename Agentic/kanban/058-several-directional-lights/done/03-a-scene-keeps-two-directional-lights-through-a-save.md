# 03 — A scene keeps two directional lights through a save
folder: authoring
after: none
decisions: 0168, 0349, 0324

## Change
Proof for `## How to test` step 6: two light rows survive a write and a read, each keeping its own
fields. Writing and reading are by description, so no code changes. Read the headers of
`authoring/include/authoring/scene_write.h`, `authoring/include/authoring/scene_read.h`,
`scene/include/scene/light_component.h` and `authoring/tests/scene_read_unsaid.c`, which already
registers scene's light in a test world.

- `authoring/tests/scene_lights.c` (new), set up as `scene_read_unsaid.c` is. It builds a world with
  two entities, each with a transform and a light:
  - a sun: white, intensity 3, a fill, bounces 1, `cast_shadows` true;
  - a moon: faint blue, intensity 0.2, no fill, bounces 0, `cast_shadows` false.

  It writes the world with `voe_authoring_scene_write` and reads the text into a fresh world with
  `voe_authoring_scene_read`. Then it checks:
  1. the fresh world has two light rows;
  2. each light's fields equal what was written, `cast_shadows` included;
  3. each light's transform rotation equals what was written.

  It also reads a file text holding one light section with no `cast_shadows` line, as a scene from
  before 049. That gives one row that casts, which is the old scene's look.

  Header points: the claim, and that the writer and reader are generic, so this test guards the
  light's description and not new code.
- `authoring/tests/tests.md`: the new file's entry, under 300 characters.

The build finds the new test by its folder; no CMake edit.

## Done when
The test `authoring/scene_lights` passes after the folder's build.
