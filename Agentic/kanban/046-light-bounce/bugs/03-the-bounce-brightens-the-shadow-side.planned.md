# 03 — The bounce brightens the shadow side

## Seen
"No, still wrong. It is emitting shadowside. When light hits a surface it bounces on the same side as the
light. Not on the backside where the shadow is. As it is now, the light is brightening the shadows near the
surface. Which is wrong."

Seen in the editor after card 25 (sun π in the tank project's `main.scene`): the box's own shadow on the
ground, close to the box, is lit up by the bounce, so shadows near a sunlit object read washed out.

## Expected
As decision 0312 says. A sunlit surface bounces light only to the side its lit face looks at:
1. Lit side: the ground at the foot of a sunlit, strongly coloured box's lit face reads at least 12/255 more
   in the box's colour than open ground 8 m away, fading with distance (0310, unchanged).
2. Shadow side (amended by 0315: 8/255, SHADOW SIDE 4/255): the ground inside the box's sun shadow, right at the foot of its shadowed face, reads within
   2/255 in every channel of the same pixels drawn with the bounce off. The box's colour does not appear
   there at all.
3. No dark spots anywhere (bug 01).
All three hold at sun 1 and at π, in the tank project's `main.scene` loaded from the file, through the
editor's scene-view drawing, on the hardware card (RTX 4070), with the camera placed so both sides of the
box are in view. The test that measures this must fail on today's build (point 2 fails) and pass after the
fix; the card says which card it ran on and shows both runs' numbers.

## How to reproduce
1. Open the tank project in the editor.
2. Set the sun to π (or 1), shining onto one side of a coloured box.
3. Look at the box's shadow on the ground right next to the box: it is brighter than the shadow further from
   the box, and tinted in the box's colour.
4. Turn the bounce off (or compare with a build before 046): the same shadow is darker and untinted there.
