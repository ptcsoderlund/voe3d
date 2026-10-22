# 04 — The forest floor bounces light as if it had no trees

## Seen
"We have planned for light falloff on bounces right? I don't want leaves to have the same strength as
something closer to the ground."

With the sun's bounces at 1, the painted trees' leaves turn evenly green from the bounce, high in the crown as
strongly as near the ground, and the ground in the trees' shadows lifts the forest as if it were sunlit grass.
The trees' shade is in the view's cascades but not in the bounce sun maps (0442 point 5), so the bounce relights
its probes from a floor with no trees on it.

## Expected
Decision 0450. The trees are drawn into the bounce sun maps as into the cascades, so the forest floor is in
their shade in the bounce too, and the canopy takes the little bounce a shaded floor gives. No falloff with
height; captures and point shadows unchanged.

## How to reproduce
1. Open the lumberhill project's scene with `Hill.landscape` and its painted trees of `tree asset.glb`.
2. Select the Light and set bounces to 1.
3. Fly in among the trees: the leaves are green from below at every height, as strongly as with no trees around.

## How to test
1. With bounces 1, the leaves over the shaded forest floor take only a faint bounce; leaves at the forest's
   sunlit edge, over open grass, still take a clear green bounce.
2. Erase the trees in one patch with the brush: the bounce there brightens back to open-grass strength once the
   probes are relit, with no hitch.
3. Bounces 0: the picture is exactly as before this change.
