# 01 — Sky strength lights shiny surfaces, even in shadow

## Seen
"When changing strength on sky, it changes the light on the imported model. Not on the terrain and not on
the primitive types (shapes)." After metal was added to the cube shape, it changed too, "even when in
shadow." With sky strength 0, the stable and the cube are almost black on their shadowed sides. With
strength 1, both are clearly lit. "It looks really bad." This is the sky's reflection (0411 point 5) acting
as light, and only lights may light (0411 point 1). Decision 0423 settles how it must work.

## Expected
- Moving the sky's strength between 0 and 1 changes only the sky backdrop and the haze. The stable, the
  cube and the terrain do not change at all.
- A shiny surface still shows the sky in its reflection, but only as bright as the light reaching it. In
  sunlight it is no brighter than the sky itself. In the shadow of the stable, with no fill, the metal cube
  is dark.

## How to reproduce
1. Open lumberhill_project. It has the hill, the imported wooden_stable, a sun and a Sky row.
2. Give Cube 2 a material with metal 1 (MetalMaterial.material) and place it in the stable's shadow.
3. Select Sky and move strength from 0 to 1.
4. Seen: the stable's and the cube's shadowed sides go from nearly black to lit.
   Expected: neither changes. Only the sky behind them brightens.
5. Move the cube into the sun. It shows the sky in its reflection, no brighter than the sky.
