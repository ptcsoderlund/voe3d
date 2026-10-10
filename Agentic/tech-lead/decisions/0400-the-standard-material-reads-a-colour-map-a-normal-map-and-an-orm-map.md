# 0400 — The standard material reads a colour map, a normal map and an ORM map
date: 2026-10-09
by: tech-lead

## Decision
The material Create → Material makes is the engine's standard material: the `lit` (or `unlit`) shader
with values and three picture slots. It is what you use when you want no shader of your own; shaders built
visually in a shader editor come later (0189) and are a separate kind of material. The three slots
are named `colormap`, `normalmap` and `ormmap`, both as the `.material` file's keys and as the Inspector's
slot labels ("Colormap", "Normalmap", "ORMmap"). The ORM map packs one value per channel: red is
ambient occlusion, green is roughness, blue is metal, as glTF and Blender pack them. Green times
Roughness and blue times Metal give the surface's roughness and metal, as the metal-roughness slot reads
them today. Red darkens only the light that does not come straight from a light: the fill and the bounce.
It never darkens direct light or shadows. An empty ORM slot reads as occlusion 1, roughness and metal
from the sliders. A glTF model's occlusion texture is read the same way, from its red channel. This
replaces the `roughness_map` slot. A greyscale roughness picture is no longer a slot of its own: it is
packed into an ORM picture outside the engine.

## Reasoning
- ORM over separate roughness, metal and occlusion slots: one picture and one fetch per surface, the
  layout Blender's glTF export and most texture tools already write, and the same slot glTF models use.
- Occlusion only on indirect light: that is what a baked occlusion term means; darkening the sun's light
  too would double the shadows. It is a texture of the world, not a screen effect, so 0328 is untouched.
- Separate greyscale slots with the engine packing them: more slots, more fetches, and work a paint tool
  already does. An editor "pack these three pictures into an ORM" tool can come when it is missed.
- The names are the human's call: `colormap`, `normalmap`, `ormmap`.

## Replaces
Amends 0399 points 1, 2 and 8 (the `roughness_map` key and slot, the roughness-map reading, the slot
labels). Nothing else in 0399 changes.
