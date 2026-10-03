# 0310 — The bounce is proven by sight in the tank scene, on a real card
date: 2026-10-01
by: tech-lead

## Decision
Answers 046's needs-decision. Bug 01 is not fixed and its Expected stands. 046 closes only when the bounce
can be seen in the editor, in the tank project's own `main.scene` as the sponsor has it, with the sun at 1
and at π: the ground at the foot of a sunlit, strongly coloured box reads plainly tinted in that box's
colour, at least 12/255 more in the box's strongest channel than open ground 8 m away, fading with
distance. The bounce is too weak to see, not only clipped, so its strength is raised until that holds;
0307's "cheap cheats over physics" allows a gain beyond what is physically right. Making the dark spots
go away still counts. The proof has to measure pixels on a hardware Vulkan card (this machine has an
RTX 4070), through the same draw calls the editor makes for a scene view, with the scene loaded from the
project file and not rebuilt by hand. A pass on llvmpipe or any software card never shows a look is
right. A card that measures pixels names the card it ran on. A sun of 9 clipping to white is not 046's
problem: tone mapping becomes a work order of its own after 046. `3d/shadows`' 100 km case draws both
of its pictures with the bounce off, so it tests only the shadow cascades.

## Reasoning
The sponsor's screenshot at sun 1 shows no tint, so lowering the sun (needs-decision option 1) would not
fix the bug. Accepting the clip (option 3) leaves the feature invisible. Tone mapping now (option 2)
would stop sun 9 clipping but still not make a weak bounce show. Headless tests on scenes built by hand
passed while the editor showed nothing. That is why the proof has to be the real scene, through the
editor's path, on a real card.

## Replaces
Nothing. Amends 0308 point 4, the bounce's strength.
