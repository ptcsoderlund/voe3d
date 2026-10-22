# 096 — Models blend into the ground

## What
A model placed on the terrain, a rock, a log or a wall, can take on the ground's painted layers where it
meets the ground, fading into its own material higher up, so no hard seam shows where it enters the
ground. Like CryEngine 5.6. Each placed model has a "Blend with ground" switch in the Inspector and a
height for how far up the fade reaches. Each foliage kind has the same switch and height in the Landscape
panel. Both are off by default, so the house's walls and the concrete base keep a hard edge unless I turn
it on. Grass never blends. Moving the model, sculpting or repainting under it changes the blend at once.
The blend looks the same in the editor's view, in Play and in a shipped build, and costs little enough
that the hill stays smooth.

## Why
A rock half sunk in moss looks pasted on when its edge meets the ground in a hard line. Blending is what
makes the rocks, logs and buildings look as if they stand in the hill (sponsor, 2026-10-22: "It looks
awesome.").

## How to test
1. Place a rock half into the painted hill. With the switch off, the seam is as it is today.
2. Turn on Blend with ground. The rock's foot takes on the moss, grass or dirt painted under it and fades
   into its own stone higher up. No line shows where it enters the ground, in sun and in shade.
3. Raise and lower the height. The fade reaches further up the rock and back down.
4. Drag the rock onto a patch of a different layer. Its foot takes on that layer at once. Repaint under it
   and sculpt the ground up around it: the blend follows.
5. Leave the house's concrete base off. Its edge against the ground stays hard.
6. Turn the switch on for one of my tree kinds in the Landscape panel. The trunks' feet blend into the
   ground. Fly low through the forest: it stays smooth, and the frame breakdown shows what the blend costs.
7. Save, close and reopen the project. The switches and heights are as I left them.
8. Press Play, and ship: the blend looks as it does in the editor's view.
