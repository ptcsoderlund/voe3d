# 087 — Models look as they do in Blender

## What
A model I export from Blender, or download, looks in the editor as it does in Blender's material
preview. Its textures sit, repeat and are read as the file says: colour textures as colours, normal,
roughness and metal maps as data. Normal maps give the right bumps, also on mirrored halves. Metal,
roughness, emission and vertex colours show. Cut-out parts, like leaves on a card, have hard edges that
stay solid when far away. A surface marked double-sided is seen from both sides. A model's materials
come in as materials from 084 that I can open and edit, and models from different files that share a
shader do not each cost their own draws (0375).

## Why
The house, the tower and the trees are my own models, or ones I choose. They have to look like what I
made.

## How to test
1. Export the loghouse from Blender as `.glb` and import it. Place it on the hill. Beside Blender's
   material preview, it looks the same at a glance: logs, bark, metal fittings.
2. Fly close to a log. Its bumps are lit from the sun's side, also on any mirrored part.
3. Import a tree with leaf cards. Leaf edges are crisp up close. Fly back until it is small: the leaves
   do not thin away to nothing. Fly around it: leaves show from both sides.
4. Open one of the house's materials in the Inspector and change its roughness. The house changes.
5. Import a downloaded model with emission. The emissive part glows.
6. Save, close and reopen. Press Play: the same in the game window.
