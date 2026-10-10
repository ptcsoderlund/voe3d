# 01 — The material's slots are colormap, normalmap and ormmap

## Seen
The material in the Inspector has three picture slots: Colour, Normal and Roughness. The roughness slot
takes a greyscale roughness picture. I want the standard material to take an ORM picture instead:
occlusion, roughness and metal, one per channel in red, green and blue. Decision 0400.

## Expected
- The slots read "Colormap", "Normalmap" and "ORMmap", and the `.material` file's keys are `colormap`,
  `normalmap` and `ormmap`.
- A picture dropped on ORMmap gives the surface its roughness from green and its metal from blue, each
  times its slider. Its red darkens the fill and bounce light in the creases, but not the sun's direct
  light or the shadows.
- With ORMmap empty, the surface looks exactly as it did with no roughness map.
- A glTF model whose material has an occlusion texture shows that occlusion the same way.
- Renaming or moving an ORM picture is followed, as the other maps are. Undo, save, reopen and Play keep
  the ORM map like the other two.

## How to reproduce
1. Import a dirt texture set with a colour, a normal and an ORM picture (red occlusion, green roughness,
   blue metal) into Assets.
2. Create → Material, named Dirt. The Inspector shows the slots Colormap, Normalmap and ORMmap.
3. Drag the three pictures into their slots, and give Dirt to a cube under a sun with Fill turned up.
4. The creases of the dirt are darker in the fill light, and its shine follows the green channel.
5. Turn the sun's fill down to 0: the creases look no darker than the rest in the sun's direct light.
6. Open `Assets/Dirt.material` in a text editor: the keys are `colormap`, `normalmap`, `ormmap`.
7. Clear ORMmap with ×: the cube looks like a material with no map but the sliders.
