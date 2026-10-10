# 51 — The Inspector's slots are Colormap, Normalmap and ORMmap
folder: editor
after: 47, 50
decisions: 0168, 0400
read: feature.md

## Change
0400 in the editor: the open material's three slots are labelled "Colormap", "Normalmap" and
"ORMmap", and every use of the file's fields follows card 48's names. Dropping, clearing, undo, save,
follow on rename and the cook keep working for the third slot as for the other two; only names change.

- `editor/src/inspector_material.c` — the three row labels become "Colormap", "Normalmap", "ORMmap";
  the field uses become `colormap`, `normalmap`, `ormmap`.
- `editor/src/inspector_material.h` — the header and the comment on the map rows name the new slots.
- `editor/src/game_tree_materials.c` — the cooked initializer writes `.colormap`, `.normalmap`,
  `.ormmap` (it is compiled against `game/materials.h`'s type, so the names must match).
- `editor/src/interface_read.c`, `editor/src/material_steps.c`, `editor/src/assets_drag.c` — the
  map compares and lists use the new fields.
- `editor/src/src.md` — the `inspector_material` lines name the new slots if they name the old.

## Done when
`grep -rn 'colour_map\|normal_map\|roughness_map' assets 3d game editor` prints nothing,
`grep -n '"ORMmap"' editor/src/inspector_material.c` finds the label, and the editor builds.

Human, the bug's steps: import a dirt set (colour, normal, ORM) into Assets; Create → Material, Dirt:
the Inspector shows Colormap, Normalmap, ORMmap; drop the three pictures, give Dirt to a cube under a
sun with Fill up: the creases are darker in the fill, the shine follows green; Fill to 0: the creases
are no darker in the sun's direct light; `Assets/Dirt.material` holds `colormap`, `normalmap`,
`ormmap`; clear ORMmap with ×: the cube looks like a material with sliders only; rename the ORM
picture, undo, save, reopen and Play keep it.
