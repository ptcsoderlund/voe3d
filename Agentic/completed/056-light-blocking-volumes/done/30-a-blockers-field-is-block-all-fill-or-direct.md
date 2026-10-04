# 30 — A blocker's field is Block: All, Fill or Direct
folder: scene
after: 29
decisions: 0168, 0352, 0353

## Change
0353 point 1 and the blocker's half of point 2. The names only; the numbers stay. 3d and game still
spell the old names until cards 32 and 33; this card does not touch them. Read the headers of
`ecs/include/ecs/component.h` (the former names section) and the files below.

- `scene/include/scene/light_blocker_component.h`:
  - The constants become `VOE_SCENE_LIGHT_BLOCKER_ALL` 0u, `_FILL` 1u, `_DIRECT` 2u (old ones
    gone); `voe_scene_light_blocker_kind_names` becomes `voe_scene_light_blocker_block_names`.
  - The field list's `kind` becomes `F(uint32_t, block, UINT32)`; the names list follows.
  - Header points: the paragraph on kinds names Block and its three values by what each stops
    (0352): All keeps every outside light out and every inside light in; Fill keeps the
    directional fill out and lets direct light through openings; Direct stops direct light only.
    All is 0, so a new, zeroed or pre-field blocker is All; a file saved with `kind` is read by
    its former name (0353). Every other "kind", "Room", "Indoors", "Wall" in the header renamed.
- `scene/src/light_blocker_component.c`: the names "All", "Fill", "Direct"; header line.
- `scene/src/light_blocker_system.c`:
  - The default row's `.block = VOE_SCENE_LIGHT_BLOCKER_ALL`; the assert and the drain's refusal
    compare against `_DIRECT`, their messages say "block" and "past Direct".
  - Registration calls `voe_ecs_component_formerly_set` with one file-scope pair
    ("kind", "block").
  - Header points renamed; one says the former name is set.
- `scene/include/scene/light_blocker_system.h`: comments naming the kind or its old values say
  Block and the new values; the register call's comment says it sets the former name.
- `scene/tests/light_blocker.c`: its cases in the new names (default All; a replace to Direct; a
  block of 3 refused, row kept; with descriptions on, the field is named `block` and its names
  are All, Fill, Direct); a case that after registration `voe_ecs_component_formerly` hands back
  one pair, "kind" to "block".
- `scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`: entries naming the kind or its
  values say Block, All, Fill, Direct. Each at most 300 characters.

## Done when
The test `scene/light_blocker` passes after the folder's build, and
`grep -rnE "LIGHT_BLOCKER_(ROOM|INDOORS|WALL)|kind_names" scene` prints nothing.
