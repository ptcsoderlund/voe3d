# 095 — Each material chooses what grows on it

## What
Grass is no longer a thing of its own: it is a foliage layer like the trees, ferns and rocks, one whose
look is grass blades instead of a model. Every foliage layer has the same settings, which I choose per
layer: Cast shadows, its base size, the scatter sliders, and (from 097) Blend with ground.

Each material painted on the ground chooses which foliage layers grow on it by themselves, each with its
own density. Where that material is painted, those layers grow: thick where the paint is strong, thin
where it fades. Repainting the ground changes them at once. A material that chooses nothing grows nothing.
What grows by itself belongs to the material: the Erase brush leaves it alone, and the way to take it
away is to paint the material away or untick the layer on it.

Painting foliage by hand works as now, for every layer, grass blades too.

This replaces 094's Grass section; how grass blades look and fade stays as 094 made them.

## Why
I decide what grows where, per material, and the engine treats no foliage as special (0473).

## How to test
1. Open the painted hill. There is no Grass section; grass is a layer in the foliage list, its look grass
   blades. On the grass material it is ticked to grow; grass stands where grass is painted, as before.
2. Untick grass on the grass material. The grass goes at once. Tick it again: it comes back.
3. Tick grass on the moss material too, with a lower density. Thinner grass grows on the moss.
4. Tick a fern layer on the moss material. Ferns grow on the moss by themselves, none on dirt or rock.
   Paint moss into a new patch: ferns and grass grow there as I paint.
5. Use Erase over the grown ferns: they stay. Erase over ferns I painted by hand: those go.
6. Turn Cast shadows off on the fern layer: ferns stop casting shadows but are still shadowed. Turn it
   on for grass: the blades cast shadows.
7. Paint grass blades by hand onto the dirt, where no material grows them. They stand there.
8. Save, reopen, press Play, and ship: the same foliage in all of them. Flying low over the hill stays
   smooth on my laptop.
