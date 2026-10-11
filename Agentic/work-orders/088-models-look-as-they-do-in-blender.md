# 088 — Models look as they do in Blender

## What
A model I export from Blender, or download, looks in the editor as it does in Blender's material
preview. Importing the `.glb` gives me one material of our own per material in the file, in plain colours
first; right-clicking the `.glb` and choosing Import textures writes its pictures as PNGs and puts colour,
normal, ORM and emission into those materials (0403, 0404). Its textures sit, repeat and are read as the
file says: colour textures as colours, normal, roughness and metal maps as data. Normal maps give the
right bumps, also on mirrored halves. Metal, roughness, emission and vertex colours show. Cut-out parts,
like leaves on a card, have hard edges that stay solid when far away. A surface marked double-sided is
seen from both sides. Which material each part wears belongs to the model, so every copy I place wears
the same, and the Material 1–8 rows are gone from the Inspector. Models from different files that share a
shader do not each cost their own draws (0375).

## Why
The house, the tower and the trees are my own models, or ones I choose. They have to look like what I
made.

## How to test
1. Export the loghouse from Blender as `.glb` and import it. A material for each of its Blender materials
   appears beside it in Assets. Place it on the hill: it shows in plain colours, no textures.
2. Right-click the `.glb` and choose Import textures. PNGs appear beside it, and the house now looks, beside
   Blender's material preview, the same at a glance: logs, bark, metal fittings.
3. Fly close to a log. Its bumps are lit from the sun's side, also on any mirrored part.
4. Import a tree with leaf cards and import its textures. Leaf edges are crisp up close. Fly back until it
   is small: the leaves do not thin away to nothing. Fly around it: leaves show from both sides.
5. Place a second house. Open one of the house's materials in the Inspector and change its roughness. Both
   houses change. Select a house: there are no Material 1–8 rows.
6. Import a downloaded model with emission and import its textures. The emissive part glows.
7. Change a material's colour map by hand, then import the `.glb` again. The material is back to the
   file's plain values and the PNGs are still there; I assign the map again by hand.
8. Save, close and reopen. Press Play: the same in the game window.
