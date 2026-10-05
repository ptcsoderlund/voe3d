# 066 — The Bistro loads and draws

## What
I export Amazon Lumberyard's Bistro from Blender as two `.glb` files, exterior and interior, import both in the
editor's Assets panel and place them in one scene. Every object and every material of both parts comes in, at
the right place and size. The files stay in my project folder and out of git (0368). This is 0.3's first
milestone (0368).

Each material looks the way the file describes it: base colour, normal map, roughness and metal, and emission
glowing on lamps and signs. Plants, grates and other cut-outs have hard edges where their alpha cuts them. A
surface Blender marks two-sided is drawn from both sides, so leaves and awnings have no missing faces. A
one-sided surface is still culled as today. Windows and bottles are glass. You see through them, tinted by the
glass's colour, to what lies behind, and they still read as glass.

Textures are compressed on the GPU, so the whole Bistro fits in video memory with room to spare. The frame
breakdown (062) shows how much video memory the scene's textures take. Only what the camera can see is drawn,
so looking at a wall costs much less than looking down the whole street.

Loading the scene shows the splash with its progress (0362), and the editor stays responsive with the whole
Bistro in it, including the Scene list. A part of the file the engine cannot read is named in the Errors panel
with the object it belongs to. The rest of the scene still loads.

Lighting, exposure, sky, fog and anti-aliasing are later milestones. Here the Bistro is lit by today's sun and
lights, and it only has to look right in those terms.

## Why
0.3 is the Bistro (0368), and nothing else in it can be judged until the scene is in the engine with its
own materials.

## How to test
1. In Blender, export the Bistro's exterior and interior as `.glb` files with the default glTF settings.
   Import both in the Assets panel. Neither import is refused. If a part is skipped, the Errors panel names
   it and why.
2. Place both models in a new scene. The interior sits inside the exterior's building, with no gap or
   offset. Save, close and reopen the editor. The scene opens with the splash showing progress and looks
   the same.
3. Fly along the street. Compare with Blender's material preview: the colours, the bumps from normal maps
   and the shine on metal and wet-looking surfaces match. Lamp shades and signs glow.
4. Look at the trees, plants and hanging flowers up close. The leaves have crisp cut edges, with no grey or
   black boxes around them, and you see them from both sides as you fly around them.
5. Look through a café window from the street. You see the interior through the tinted glass. Look out
   from inside: the street shows through. Bottles behind the bar are see-through as well.
6. Open the frame breakdown. The textures' video memory is shown and is well under the card's total.
7. Face down the whole street, then fly up to a wall so it fills the view. The frame total drops by a
   large factor.
8. Scroll the Scene list and select items in it and in the view. The editor does not stall.
9. Press Play and check steps 3–5 again in the game window.
