# 01 — Zoom in jumps, zoom out glides

## Seen
Scrolling the camera farther out is smooth, but scrolling it nearer jumps straight to the new
distance. The spring arm treats a shorter distance from the wheel the same as a wall: it snaps in
at once. Only the way back out eases, at the arm's 10 m/s.

## Expected
The wheel zoom eases both ways: nearer and farther glide to the distance I scrolled to, at the
same speed. Only when something solid comes between the camera and the capsule does the camera
snap in at once, as it does now, and it still eases back out when the way is clear.

## How to reproduce
1. Open `examples/coin_game/` in the editor, press Play, click Start.
2. Stand in the open, away from walls. Scroll out a few notches: the camera glides out.
3. Scroll in a few notches: the camera jumps in instead of gliding. It should glide as in step 2.
4. Run next to a wall and turn the camera so the wall would be between them: the camera must
   still come in front of the wall at once, not glide through it. Turn away: it glides back out.
