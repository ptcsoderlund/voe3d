# 11 — The editor's sun is placed, and kept single
folder: editor
decisions: 0168, 0273, 0223

## Change
Needs cards 01–08. After this card the editor builds again.

- `editor/src/project.c`, the untitled scene: the light entity gets a transform at (0, 4, 0),
  rotation `voe_scene_light_facing` of the `LIGHT_X/Y/Z` direction, scale one, before its
  light of white and `LIGHT_INTENSITY`, no fill. Rewrite the "LIGHT HAS NO TRANSFORM" comment
  and the `LIGHT_*` comment: the direction is turned into the sun's rotation, and the sun
  stands where its marker is seen.
- `editor/src/scene.c`, `voe_editor_scene_duplicate`: does nothing for an entity with a light,
  as for the camera (one sun, 0273); Delete stays allowed. `editor/src/scene.h`: the sentence
  on what Duplicate refuses.
- `editor/src/inspector.c`, the Duplicate and Delete row: no Duplicate button for an entity
  with a light; Delete still drawn. Its comment says why.
- `editor/src/src.md`: the `project.c` and `scene.c` lines if they name what is refused or the
  untitled light.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`build/debug/editor/voe_editor examples/coin_game --capture <scratch>/coin.png` exits 0 with a
PNG written (a scratch folder of your own).
