# scene

The public headers, one entry each; the fuller account of every one of these
stays on `scene/scene.md`.

- `transform_component.h` — position, rotation and scale, and the matrix the
  three of them become.
- `transform_system.h` — the intent that moves one, and the direct call that
  creates one.
- `parent_component.h` — the entity a thing hangs under, and the walks of the tree.
- `parent_system.h` — registering the parent table.
- `prefab_component.h` — the prefab a placed copy is, and the copy a part was
  made for.
- `prefab_system.h` — registering the two prefab tables.
- `camera_component.h` — eye, yaw, pitch, field of view, the two planes, and the
  view matrix.
- `camera_system.h` — the two camera intents, absolute and relative.
- `identity_component.h` — a 64-bit id, a name and whether its row is folded on
  the entities a person authored.
- `identity_system.h` — the intent that renames one, and the direct call that
  creates one.
- `light_component.h` — the sun: which way its light travels, its colour and its
  strength.
- `light_system.h` — the intent that turns it, and the direct call that creates
  one.
- `point_light_component.h` — a lamp: its colour, intensity, range and flash,
  and the strength it shines with now.
- `point_light_system.h` — the replace and flash intents, the direct call that
  creates one, and the run that fades it.
- `light_blocker_component.h` — a box that stops light by its Block, All, Fill
  or Direct: its size along its transform's axes and its Block.
- `light_blocker_system.h` — the replace intent, the direct call that creates
  one, and the run that drains it.
