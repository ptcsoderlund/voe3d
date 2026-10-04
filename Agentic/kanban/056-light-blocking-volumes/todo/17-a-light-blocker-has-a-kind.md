# 17 — A light blocker has a kind
folder: scene
after: none
decisions: 0168, 0348, 0350

## Change
The row's `kind` (0350 point 1), named as the collider's kind is. Read
`physics/include/physics/collider_component.h` for the named-UINT32 pattern, and the files below.

- `scene/include/scene/light_blocker_component.h`: `VOE_SCENE_LIGHT_BLOCKER_ROOM` 0u, `_INDOORS`
  1u, `_WALL` 2u; `extern` `voe_scene_light_blocker_kind_names[3]`; the field list gains
  `F(uint32_t, kind, UINT32)` after `size`; the struct is described with
  `VOE_BASE_DESCRIBE_STRUCT_NAMED` and a names list for `kind`. Header points: what each kind
  does to light (0348, one line each); Room is 0, so a new blocker, a zeroed one and a file saved
  before the field are Rooms; the rule is render's.
- `scene/src/light_blocker_component.c`: the names, "Room", "Indoors", "Wall".
- `scene/include/scene/light_blocker_system.h`, `scene/src/light_blocker_system.c`: `add` asserts
  on a kind past Wall and the drain refuses one, keeping the row, as for a bad size; the default
  row's kind is Room. Header points changed to say so.
- `scene/tests/light_blocker.c`: the default row's kind is Room; a replace to Wall changes it; a
  kind of 3 is refused and keeps the row; with descriptions on, the field is named and its names
  are the three.
- `scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`: the entries for what changed name
  the kind. Each at most 300 characters.

## Done when
The test `scene/light_blocker` passes after the folder's build.
