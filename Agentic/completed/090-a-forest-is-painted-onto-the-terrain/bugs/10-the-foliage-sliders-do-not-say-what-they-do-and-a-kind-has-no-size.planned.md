# 10 — The foliage sliders do not say what they do, and a kind has no size of its own

## Seen
"It says 'size' but in reality it is size randomizer. Also, I have no idea what Lean% means."
The Size slider only sets how much each tree's size varies, and there is no way to make all the
trees of one kind bigger or smaller without going back to Blender. Lean is the tilt toward the
ground's slope, but nothing says so.

## Expected
Per decision 0459:
- The brush sliders read **Size ±%**, **Turn ±°** and **Tilt with slope %**.
- Each kind's row in the Foliage section has a base size, 1× by default. Changing it resizes every
  tree of that kind already painted, as well as the ones painted later. It is kept on save.

## How to reproduce
Open lumberhill and pick a thing that wears the landscape. Under Foliage the sliders read Size,
Turn and Lean, and a kind's row has nothing but its name: no control makes every pine bigger.

## How to test
1. Open lumberhill and pick a thing that wears the landscape. Under Foliage, the sliders read
   Size ±%, Turn ±° and Tilt with slope %.
2. Every kind row shows a base size of 1×, and the forest that was already painted looks exactly
   as it did before.
3. Set the pine's base size to 2×. Every pine already painted doubles in size, standing in the same
   spot and pointing the same way. The other kinds don't change.
4. Scatter more pines with Size ±% at 0. Every new pine matches the 2× pines exactly.
5. Set Size ±% to 50 and scatter again. The new pines vary from about 1× to 3×.
6. Set Tilt with slope % to 100 and scatter on a steep bit. The trees stand square to the slope.
   At 0 % they stand straight up.
7. Save, close the editor and reopen it. The pine's base size is still 2× and the forest looks
   the same.
8. Undo the base size change. The pines go back to 1×.
