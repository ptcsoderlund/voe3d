# 085 — Materials are made and edited in the editor

## What
A material is an asset of its own in the Assets panel (0377). Create → Material makes one. I pick its
shader, and set its values and textures in the Inspector: colour, roughness, metal, a normal map, and how
often its textures repeat. Textures are dragged from the Assets panel into its slots. Anything using the
material changes at once when I edit it. A material can be given to a placed shape or a model's part.
Edits are undoable and are saved in the project.

## Why
0374 has me edit materials at least far enough to make the ground's layers. Those layers are 086's input.

## How to test
1. Import a dirt texture set (colour, normal, ORM) into Assets. Create → Material, named Dirt, and
   drag the three textures into its slots.
2. Give Dirt to a cube. The cube shows the dirt, with bumps in the light.
3. Change the repeat from 1 to 4. The texture tiles four times across each face at once.
4. Change roughness and colour. The cube changes as I drag. Undo goes back.
5. Duplicate Dirt, give the copy a different tint, and give it to a second cube. Each cube keeps its own.
6. Rename Dirt. The first cube still shows it.
7. Save, close and reopen. Both materials and cubes are as I left them. Press Play: the same.
