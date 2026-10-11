# 02 — The ORM map alone sets roughness and metal

## Seen
"It looks to me like roughness is way too shiny." The hill in lumberhill uses `Hillmat.material` with
`dirt_orm.png` in the ORMmap slot. The map's roughness (green) runs 0.43–0.75 and should look like dry
dirt, but the ground looks wet and glossy. The Roughness slider is at its default of 0.5 and is multiplied
into the map, so the surface really gets 0.22–0.37. The Metal slider multiplies the map's blue the same
way, so a metal map with Metal at 0 shows no metal at all.

## Expected
Decision 0401. With an ORM map assigned, roughness and metal are exactly what the map carries, and the
Roughness and Metal sliders change nothing. They are greyed out and cannot be dragged while the slot is
filled. When the slot is emptied they come back with the values they had and work as before. A glTF model
with a metallic-roughness texture shows that texture's roughness and metal whatever its factors say.

## How to reproduce
1. Open the lumberhill project and look at the hill up close, with the sun low and in front of you.
2. Select `Assets/Hill/Hillmat.material`. Roughness shows 0.5 and ORMmap shows `dirt_orm.png`.
3. Today the ground is glossy, and dragging Roughness to 1.0 makes it matte. Dragging it to 0.1 makes it
   look like a mirror.

## How to test
1. Do steps 1–2 above. The ground looks like dry, matte dirt without touching anything.
2. The Roughness and Metal sliders are greyed out; trying to drag them does nothing and the ground does not
   change.
3. Clear the ORMmap slot. The sliders light up again, still showing 0.5 and 0, and dragging Roughness
   changes the shine as before.
4. Put `dirt_orm.png` back in the slot. The ground is matte again whatever the slider was left at.
5. Press Play: the hill in the game looks the same as in the editor.
6. A material with metal 0 on its slider and an ORM map whose blue is white looks like metal.
