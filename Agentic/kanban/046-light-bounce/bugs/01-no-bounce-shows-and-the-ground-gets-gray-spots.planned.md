# 01 — No bounce shows, and the ground gets gray spots

## Seen
"Something has happened. The ground now have gray spots on it. But i see no reflections. [...] I dont see
any light bounce"

Screenshot from the editor on Windows: `Skärmbild 2026-10-01 141004.png` in the repo root. A saturated red
box and a green box stand in sunlight on beige ground. The ground at the foot of the red box is the same
beige as the open ground, with no red tint. The ground shows faint dark gray blotches, mostly near the
bases of the boxes.

## Expected
Sunlit coloured surfaces spill their colour onto the surfaces near them: the ground beside the red box
reads visibly redder than ground a few metres away, fading with distance, as the bounce test shows
headless (177 against 160 in red, 1 m from a red wall). The bounce never darkens a surface below what it
read before the feature: no gray or dark blotches appear anywhere, in sun or in shadow.

## How to reproduce
1. Build the editor on Windows and open a scene with a strongly coloured box standing on light ground in
   sunlight (the scene in the screenshot).
2. Look at the ground right at the foot of the coloured box: no tint.
3. Look over the open ground: faint dark gray spots that were not there before feature 046.
