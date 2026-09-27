# 02 — The fill colours the sunlit parts too

## Seen
"Fill color and fill intensity is filling everything, not just shadows." Raising the fill
brightens and tints the sunlit surfaces as well as the shadowed ones.

## Expected
As decision 0275: the fill lifts only the shade. Surfaces in full sun look the same whatever
the fill is; shadowed sides and surfaces facing away from the sun get lighter and take the
fill's colour. The shadows stay. The game looks the same when played (step 8).

## How to reproduce
1. Open `examples/coin_game` in the editor and select the sun.
2. Set `fill_colour` to a strong blue and raise `fill_intensity` to 1.
3. Look at a surface in full sun: it turns bluer and brighter. It should not change.
