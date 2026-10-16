# 08 — I want to set each LOD's reduction and swap

## Seen
One Strength and one Swap for the whole model is too coarse. A wall and a handle on a gate should behave
differently, and I want to decide how hard each LOD is decimated and when it swaps in, so the drop across the
LODs can follow a curve rather than a fixed step. My own LODs from Blender swap by a plain size rule, not by
pixels like the made ones, and I cannot adjust them.

## Expected
As decision 0435 says:
- The engine still picks the LOD by its error on screen and dissolves between LODs as it does now.
- In "LODs of <file>", every LOD from LOD 2 on has its own triangle share (% of LOD 1) and its own swap
  multiplier (×1 by default; larger swaps in further away, smaller closer).
- A Blender model's own LODs are measured on import, swap by pixels like made ones and take the same
  multiplier. Their share is shown, not set.

## How to reproduce
1. Make LODs on the stable. The question offers one Strength and one Swap for all of its LODs.
2. Import a `.glb` with `_LOD1`, `_LOD2` nodes. There is nothing to set for when they swap.

## How to test
1. Make LODs on the stable. "LODs of <file>" shows a row per LOD with a share and a swap. The defaults are
   50, 25, 12, 6, 3 % and ×1 each. Make them, select a placed stable and fly away. It behaves as before.
2. Set LOD 2 to 80 % and LOD 3 to 10 % and make them. The panel and the Frame panel's Triangles show
   roughly those shares: a small drop at LOD 2, a big one at LOD 3.
3. Set LOD 3's swap to ×3. Flying away, the Inspector reaches LOD 3 much further out than before, while
   LOD 2 swaps where it did. Set it to ×0.3 instead: LOD 3 comes in close, and it may look worse.
4. Set LOD 4's swap lower than LOD 3's would allow. LOD 4 never comes in before LOD 3.
5. Change only the swaps. The model does not go through "Preparing" again, and the new swaps are used at
   once.
6. Import a `.glb` with my own `_LOD1` and `_LOD2`. Its rows show their real triangle counts with no share to
   set. Flying away, they swap later on a big model than on a small one, and their swap multipliers move
   them like a made model's.
7. Press Play and Ship. The swaps I set are used there too.
