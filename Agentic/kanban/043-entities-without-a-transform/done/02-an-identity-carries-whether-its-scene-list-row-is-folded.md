# 02 — An identity carries whether its Scene list row is folded
folder: scene
after: 01
decisions: 0168, 0300, 0302

## Change
0302 point 2. Read `scene/include/scene/identity_component.h`,
`scene/include/scene/identity_system.h`, `scene/src/identity_system.c`,
`scene/tests/identity.c` and `scene/tests/tests.md`.

- `scene/include/scene/identity_component.h`: `VOE_SCENE_IDENTITY_FIELDS`
  gains `folded`, a `bool` of kind `BOOL`, after `name`, not read-only. The
  header gains a paragraph: what it means (the Scene list shows the entity's
  children folded away, 0300), why it lives here (saved, undone and copied
  with the identity, no table for one bit, 0302), that it changes nothing
  else about the entity, and that a file without it reads as false.
- `scene/src/identity_system.c`: nothing unless the drain or the creation
  call copies fields one by one, in which case `folded` goes through whole
  like the name; the drain corrects nothing about it.
- `scene/include/scene/identity_system.h`: if its example or comments list
  the fields, add `folded`.
- `scene/scene.md` and `scene/include/scene/scene.md`: the identity entries
  say "a name and whether its row is folded".
- `scene/tests/identity.c`: a check that a replace intent with `folded`
  true lands when the system runs, and false lands back; that a new
  identity's default row is unfolded. `scene/tests/tests.md`: the entry.

## Done when
The new checks in `scene/tests/identity.c` pass in the folder's checks, and
`grep -q "folded" scene/include/scene/identity_component.h` exits 0.
