# 07 — The Inspector should count LODs from one

## Seen
"It says 0 when i am close. Only programmers start counting from 0, it would be better to have 1-6."

## Expected
The Inspector counts LODs from one: a placed model close by says **LOD 1 of 6**, the coarsest says **LOD 6 of
6**, and a model without LODs says **LOD 1 of 1**. Only what the Inspector shows changes; the LODs' names in
code, in `.parts` files and in a `.glb`'s `_LOD<n>` nodes stay as they are.

## How to reproduce
1. Make LODs on the stable with 6, select a placed stable and fly slowly away: the Inspector goes LOD 0 to
   LOD 5 of 6.

## How to test
1. Make LODs on the stable with 6. Select a placed stable close by: the Inspector says **LOD 1 of 6**. Fly
   slowly away: it says 2, 3, … up to **LOD 6 of 6**, and the Frame panel's Triangles keeps dropping at each.
2. Choose 2 on a model: the Inspector goes no higher than **LOD 2 of 2**.
3. Select a model with no LODs: the Inspector says **LOD 1 of 1**.
