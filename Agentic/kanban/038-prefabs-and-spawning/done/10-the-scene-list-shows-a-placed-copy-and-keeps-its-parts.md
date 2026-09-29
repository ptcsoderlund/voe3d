# 10 — The Scene list shows a placed copy and keeps its parts in place
folder: editor
decisions: 0168, 0283, 0281, 0282

## Change
Needs card 09. 0283 point 5, the Scene list's share.

- `editor/src/scene_list.c`, `editor/src/scene_list.h`:
  - A row whose entity has a `voe_scene_prefab` row (scene/prefab_component.h) shows, after the
    name, the prefab's file name (the path's last name), in the theme's secondary text role, so
    it reads as a prefab at a glance. Parts are listed as today, under their root.
  - A press on a part's row (a `voe_scene_prefab_part` row naming another entity) selects it but
    never starts a drag. A drag never targets a part's row (no rim, no drop), so nothing is
    parented under a part. A placed copy's root drags, and is dropped, as any row.
  - The header says both, and why (0283 point 5: a part is the prefab's, and a thing under a part
    would be saved pointing at something no file holds).
- `editor/src/src.md`: the two entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's,
on the card 09 setup: the copy's row shows `t.prefab`; its turret's row selects and does not drag;
another thing dragged over the turret's row gets no rim and does not nest there.
