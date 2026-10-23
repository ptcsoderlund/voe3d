# 095 — Each material chooses what grows on it

## What
Every foliage layer in a landscape's foliage list is a model, and every one has the same settings, which I
choose per layer: Cast shadows, its base size, the scatter sliders, and (from 097) Blend with ground. No
layer is special: grass, if I want it, is a layer whose model is a grass tuft.

Each material painted on the ground chooses which foliage layers grow on it by themselves, each with its
own density. Where that material is painted, those layers grow: thick where the paint is strong, thin
where it fades. Repainting the ground changes them at once. A material that chooses nothing grows nothing.
What grows by itself belongs to the material: the Erase brush leaves it alone, and the way to take it
away is to paint the material away or untick the layer on it.

Painting foliage by hand works as now, for every layer.

## Why
I decide what grows where, per material, and the engine treats no foliage as special (0473, 0475).

## How to test
1. Open the painted hill and add a foliage layer whose model is a grass tuft. Tick it on the grass
   material: tufts grow where grass is painted, none on dirt, moss or rock.
2. Untick it on the grass material. The tufts go at once. Tick it again: they come back.
3. Tick the tufts on the moss material too, with a lower density. Thinner tufts grow on the moss.
4. Tick a fern layer on the moss material. Ferns grow on the moss by themselves, none on dirt or rock.
   Paint moss into a new patch: ferns and tufts grow there as I paint.
5. Use Erase over the grown ferns: they stay. Erase over ferns I painted by hand: those go.
6. Turn Cast shadows off on the fern layer: ferns stop casting shadows but are still shadowed. Turn it
   back on: they cast again.
7. Paint ferns by hand onto the dirt, where no material grows them. They stand there.
8. Save, reopen, press Play, and ship: the same foliage in all of them. Flying low over the hill with
   tufts grown thick on the grass stays smooth on my laptop.
