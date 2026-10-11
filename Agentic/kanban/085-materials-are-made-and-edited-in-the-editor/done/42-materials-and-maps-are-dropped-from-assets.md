# 42 — Materials and maps are dropped from Assets
folder: editor
after: 41
decisions: 0168, 0285, 0399

## Change
0399 point 8: dragging gives a material to a thing and a picture to a map slot.

- `editor/src/assets_drag.h` / `assets_drag.c` — new outcomes for a held row released over the
  Inspector (read the header for the outcome per pointer, the ghost and the one undo step):
  - a `.material` row, the Inspector showing an entity: over a model row's Materials n field (card
    17's records) the model's `materials[n-1]` set through the model intent; elsewhere over the
    Inspector, a selected shape's `material` set through the shape intent; otherwise refused (the
    ghost's "Can't drop here", 0285). Both are a scene edit and one undo step as the model swap is.
  - a picture row (`.png`, `.jpg`, `.jpeg`), the Inspector showing an open material: over a map row
    of `inspector_material.c`'s records, that map's path in `scene->material` set to the row's path;
    elsewhere refused. Card 39's compare carries it to the table and reloads the material.
  - an emitter's texture swap over the Inspector is unchanged where it applies.
- `editor/src/inspector_material.h` / `.c` — hand back each map row's last rectangle for the drop.

Update `assets_drag.h`'s header and `editor/src/src.md`'s `assets_drag` lines.

## Done when
`grep -n 'material' editor/src/assets_drag.c` finds both outcomes, and the editor builds. Human: drag
three pictures onto Dirt's three map rows; select a cube and drag `Dirt.material` onto the
Inspector: the cube shows the dirt, bumps in the light.
