# 02 — Still no bounce at sun 1 in the tank scene

## Seen
"I added a 'Skärmbild' in root. The sun was at 1 there. I still see no reflecting lights. This is a
screenshot taken from editor in tank project. The bug is not fixed."

Screenshot: `Skärmbild_20261001_164355.png` in the repo root, from the editor with the tank project open
and the sun at intensity 1. A purple box and a grey box stand on brown ground with a tank beside them.
The ground at the foot of the purple box has the same brown as the rest of the ground, with no purple
tint.

Card 21's `3d/bounce_scene`, run on the RTX 4070 on this machine, fails THE TINT the same way it did on
llvmpipe, and the earlier cards passed headless while the editor showed no bounce.

## Expected
As decision 0310 says: in the editor, in the tank project's `main.scene`, with the sun at 1 and at π, the
ground at the foot of a sunlit, strongly coloured box reads plainly tinted in that box's colour (at least
12/255 more in its strongest channel than open ground 8 m away), fading with distance. There are no dark
spots. This is proven on a hardware card through the editor's scene-view drawing, never on llvmpipe.

## How to reproduce
1. Open the tank project in the editor.
2. Set the sun's intensity to 1.
3. Look at the ground at the foot of a coloured box in sunlight: there is no tint.
