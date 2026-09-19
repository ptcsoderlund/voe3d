# 08 — A colour is saved, and a missing field reads as its default
folder: authoring
decisions: 0168, 0190, 0191

## Change
`src/scene_write.c` / `src/scene_read.c`: a `COLOUR` field is written and read exactly as `FLOAT3` (card 02 may
already have added the case; keep it). `include/authoring/scene_write.h` and `scene_read.h` name COLOUR where they
list how each kind is spelled.

`src/scene_read.c`: a field the section does not mention now starts from that field's bytes in the type's default
row (`voe_ecs_component_default`, card 03), or zero when the type has none. It is still a warning. This is
how a scene saved before 010 opens its shapes grey rather than black. Update that sentence in `scene_read.h`.

Update `tests/tests.md`.

## Done when
The folder's check passes (`checks.sh` for `authoring`) with:
- `tests/scene_write.c` or `tests/scene_read.c`: a world holding a described type with a COLOUR field writes
  it as three numbers and reads back byte for byte.
- `tests/scene_read.c`: a section missing a field, for a type whose default row gives that field a non-zero
  value, reads that value; for a type with no default it reads zero.
