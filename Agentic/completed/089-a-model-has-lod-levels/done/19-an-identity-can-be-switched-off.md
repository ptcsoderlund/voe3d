# 19 — An identity can be switched off
folder: scene
after: none
decisions: 0168, 0427, 0428

## Change
The switch of 0427 as 0428 points 1 and 2 shape it.

`scene/include/scene/identity_component.h`:
- `VOE_SCENE_IDENTITY_FIELDS` gains `F(bool, off, BOOL)` after `folded`.
- New read, declared after `voe_scene_identity_entities`:
  `bool voe_scene_identity_off(const voe_ecs_world *world, voe_ecs_entity entity);`
  Its comment: true when the entity's own `off` or any ancestor's is set;
  ancestors walked through the parent table up to
  `VOE_SCENE_PARENT_DEPTH_MAX` links, an ancestor with no identity passed
  through; false for a dead entity, one with no identity anywhere up its
  chain, or a world that never registered the identity table; the one
  question every system asks of a row's owner (0428).
- A header paragraph after the FOLDED one, `OFF SAYS …` in the same style:
  the entity is as if it were not there (0427); children with it, their own
  switch kept; lives here for the reason folded does (0302); false is on, so
  a file without the key reads on; the drain corrects nothing about it. The
  header stays within its cap (it is 49 lines now).

`scene/src/identity_component.c`: the read. A world without the identity
table answers false without asserting: see how `scene/src/parent_component.c`
answers NULL for a table never registered, and do the same check here. The
parent read is `voe_scene_parent_get` (`scene/include/scene/parent_component.h`),
which is NULL with no parent table, ending the walk.

`scene/scene.md` and `scene/include/scene/scene.md`: the
`identity_component.h` entries add "and whether it is switched off", and
the read of the switch through its ancestors. `scene/src/src.md`'s
`identity_component.c` entry names the switch read with the others.

`scene/tests/identity.c`:
- `check_description` and any field count it checks: four fields, `off` a
  BOOL after `folded`.
- New case `a_switch_lands_and_reaches_the_children`: a world as `world_of`
  makes it plus `voe_scene_parent_register` (see
  `scene/include/scene/parent_system.h` for its call and how to parent):
  a root, its child, and a grandchild with no identity. All read on. Submit
  the root's identity with `off = true` through `voe_scene_identity_submit`
  and run `voe_scene_identity_system_run`: root, child and grandchild read
  off. The child's own `off` set and the root's cleared: root on, child and
  grandchild off. Child cleared: all on.
- New case `a_world_without_tables_reads_on`: an entity in a world with no
  identity table reads on; one with an identity and no parent table reads
  its own switch only.
- Both added to `main`. `scene/tests/tests.md`'s `identity.c` entry names
  the switch and its reach, one sentence, under 300 characters.

Nothing outside `scene/`: the authoring tests that print identities are
card 20.

## Done when
The test `scene/identity` passes after the folder's build, with both new
cases in it.
