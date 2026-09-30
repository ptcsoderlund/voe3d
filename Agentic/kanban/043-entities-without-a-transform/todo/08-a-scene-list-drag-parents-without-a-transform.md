# 08 — A Scene list drag parents without a transform
folder: editor
after: 07
decisions: 0168, 0300, 0302

## Change
0300: a bare entity can have children and be a child. Card 01 made
`voe_scene_parent_set` take a child or a parent without a transform. Read
`scene/include/scene/parent_system.h`, `editor/src/scene_list.h` and
`editor/src/scene_list.c` (`drop_target`), and `editor/src/src.md`.

- `editor/src/scene_list.c`: `drop_target` no longer refuses a held entity
  without a transform, nor a target row without one; every other refusal
  stays (dead, onto itself or under it, onto a part, already its parent,
  the heading for a root). Its comment says so.
- `editor/src/scene_list.h`: the drag paragraph says any row, with or
  without a transform, parents and takes children; a child with a transform
  under a bare row keeps its world place and its row.
- `editor/src/src.md`: the `scene_list.c` entry only if it says what is
  refused.

## Done when
`grep -c voe_scene_transform_get editor/src/scene_list.c` prints 0, and the
editor builds in the folder's checks.
