# 11 — The Inspector shows a part without editing it
folder: editor
decisions: 0168, 0283, 0217

## Change
Needs card 09. 0283 point 5, the Inspector's share.

- `editor/src/inspector.c`, `editor/src/inspector.h`: while the selected entity has a
  `voe_scene_prefab_part` row naming another entity (scene/prefab_component.h), the panel opens
  with one line saying it is part of the prefab (the root's prefab path) and that the prefab is
  opened from the Assets panel to change it; the Duplicate and Delete row, every section's Remove
  and the Add component button are not drawn; every field is drawn the way a read-only field
  (`F_READ_ONLY`, as the identity's id) already is, with no control. A placed copy's root is
  edited as any entity; its prefab path shows without a control, as the parent's field does.
  Header points: what a part shows and why nothing on it is edited (its rows come from the file
  at every read, so an edit would vanish).
- `editor/src/inspector_buttons.c`: nothing fired is carried out on a part (a guard, in case a
  stale record from the frame before reaches it).
- `editor/src/src.md`: the entries for the three files.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's,
on the card 09 setup: selecting the copy's turret shows its sections with values and no boxes,
buttons or Add component, and the line naming `Assets/t.prefab`; selecting the copy's root shows
its transform editable.
