# 04 — A prefab with no light is drawn flat

## Seen
"The prefab view is unlit, looks weird. Can we have a light source of some sort there?"

A prefab opened from the Assets panel has no light of its own, so the views draw it flat in its
material colours, with no shading or shadow.

## Expected
Per 0287, which replaces 0238:
- In the editor, a prefab or scene with no light is lit by the preview light. That is a directional
  light with a newly added directional light's settings, shadows included. It is not saved, not in
  the Scene list, and gone the moment a light is added.
- Play in the editor and the shipped game draw lit surfaces black when the scene has no light. The
  background and GUI draw as before.
- Any example level that has no light gets one so it still plays lit.

## How to reproduce
1. Open `examples/tank_game` in the editor.
2. Open a prefab from the Assets panel, for example `tank_body.prefab`.
3. The views show it flat and unshaded. Expected: shaded and casting shadows, as if a new
   directional light had just been added.
4. Back in a level, delete its light. Expected: the views look lit the same way. Press Play, and
   the level is black, with the GUI still showing. Undo to bring the light back.
