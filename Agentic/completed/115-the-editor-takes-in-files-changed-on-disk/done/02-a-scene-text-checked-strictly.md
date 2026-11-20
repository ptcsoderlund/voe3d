# 02 — Scene text checked strictly, without creating anything
folder: authoring
after: 01
decisions: 0168, 0353, 0498, 0505, 0506, 0509

## Change
- `authoring/include/authoring/scene_read.h`: add
  `bool voe_authoring_scene_check(const char *text, size_t size, voe_ecs_world *world, voe_base_arena *arena)`.
  - It returns true when `voe_authoring_scene_read` would accept the text and two more things hold:
    - every key in the section of a registered, described type names one of its fields or a former
      name (0353);
    - no ranged field holds a value outside its least and greatest. A field is ranged when
      `voe_base_ranges_find` finds it in the type's description (0498).
  - It creates nothing and changes nothing in `world`. It reads `world` only for its registered
    types, and `world` may hold entities.
  - A refusal is reported the way the reader reports one. It names the line, the section, the key
    and why; for a range, that is the range and its unit.
  - A section naming an unregistered type passes, the way the reader keeps it.
  - Header points:
    - what it adds to the reader;
    - `.prefab` text is checked by it too;
    - a file the editor opens still goes through the lenient reader (0509 point 3).
- `authoring/src/scene_read.c`, `authoring/src/field_read.c`: one validation pass serves both the
  read and the check, with the check's two extra refusals behind a flag. Keep each file under about
  800 lines.
- `authoring/tests/scene_check.c`, a new file the build finds. Register the scene types the way
  `scene_lights.c` does. Tests:
  - `an_unknown_key_is_refused_naming_it`: `texture=foo.png` in a sun's light section is refused,
    and the kept report names `texture` and its line;
  - `a_former_name_passes`;
  - `a_value_out_of_range_is_refused`: a sun's `disc_size` past `VOE_SCENE_LIGHT_DISC_SIZE_MAX`;
  - `an_unregistered_section_passes`;
  - `what_the_reader_refuses_is_refused`: a value of the wrong kind;
  - `a_written_scene_passes`: the text `scene_write` makes of a world with a sun;
  - `nothing_is_created`: the world's entity count is unchanged.
- `authoring/tests/tests.md` (the new file's entry) and `authoring/authoring.md` (the `scene_read.h`
  line): name the check.

## Meaning
- Scout: readers vary in how strict they are. The scene reader ignores, with a warning, a key that
  names neither a field nor a former name.

## Done when
`ctest --test-dir build/debug -R '^authoring/(scene_check|scene_read|scene_read_former|scene_read_unsaid|scene_lights)$'`
exits 0.
